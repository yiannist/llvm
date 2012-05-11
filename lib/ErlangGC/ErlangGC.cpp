//===-- ErlangGC.cpp - A GC plugin for HiPE compiler ----*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file implements the compiler plugin to interface the LLVM backend of
// HiPE with the Erlang/OTP runtime (e.g. defines safe points, root
// initialization etc.)
//
//===----------------------------------------------------------------------===//

#include "llvm/CodeGen/GCStrategy.h"
#include "llvm/CodeGen/GCMetadata.h"
#include "llvm/CodeGen/MachineInstrBuilder.h"
#include "llvm/Support/Compiler.h"
#include "llvm/IntrinsicInst.h"
#include "llvm/Analysis/Dominators.h"
#include "llvm/Target/TargetMachine.h"
#include "llvm/Target/TargetInstrInfo.h"
#include "llvm/MC/MCContext.h"
#include "llvm/MC/MCSymbol.h"

using namespace llvm;

namespace {
  class LLVM_LIBRARY_VISIBILITY ErlangGC : public GCStrategy {
  public:
    ErlangGC();
    virtual bool findCustomSafePoints(GCFunctionInfo &FI, MachineFunction &MF);
  };
}

static GCRegistry::Add<ErlangGC>
X("erlang_gc", "Erlang's garbage collector");

ErlangGC::ErlangGC() {
        InitRoots = false;
        NeededSafePoints = 1 << GC::PostCall;
        UsesMetadata = true;
        CustomRoots = false;
        CustomSafePoints = true;
}

// Determine that the instruction is a Tail Call */
static bool isTailCall(MachineInstr *MI) {
  const MCInstrDesc& MCid = MI->getDesc();
  return (MCid.isCall() && MCid.isTerminator());
}

static MCSymbol *InsertLabel(MachineBasicBlock &MBB,
                             MachineBasicBlock::iterator MI,
                             DebugLoc DL) {
  const TargetInstrInfo* TII = MBB.getParent()->getTarget().getInstrInfo();
  MCSymbol *Label = MBB.getParent()->getContext().CreateTempSymbol();
  BuildMI(MBB, MI, DL, TII->get(TargetOpcode::GC_LABEL)).addSym(Label);
  return Label;
}

bool ErlangGC::findCustomSafePoints(GCFunctionInfo &FI, MachineFunction &MF) {
  for (MachineFunction::iterator BBI = MF.begin(), BBE = MF.end();
       BBI != BBE; ++BBI)
    for (MachineBasicBlock::iterator MI = BBI->begin(), ME = BBI->end();
         MI != ME; ++MI)

      // Do not treat tailcall sites as safe points.
      if (MI->getDesc().isCall() && !isTailCall(MI)) {
        /* Code copied from VisitCallPoint(...) */
        MachineBasicBlock::iterator RAI = MI; ++RAI;
        MCSymbol* Label = InsertLabel(*MI->getParent(), RAI, MI->getDebugLoc());
        FI.addSafePoint(GC::PostCall, Label, MI->getDebugLoc());
      }
  return false;
}
