// lib/ErlangGC/ErlangGC.cpp - An LLVM GC plugin for HiPE LLVM backend

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
    virtual bool performCustomLowering(Function &F);
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

//------------------------------------------------------------------------------
/* Copied from GCStrategy.cpp for customInsertRootInitializers(...) */
static bool CouldBecomeSafePoint(Instruction *I) {

  // The natural definition of instructions which could introduce safe points
  // are:
  //
  //   - call, invoke (AfterCall, BeforeCall)
  //   - phis (Loops)
  //   - invoke, ret, unwind (Exit)
  //
  // However, instructions as seemingly inoccuous as arithmetic can become
  // libcalls upon lowering (e.g., div i64 on a 32-bit platform), so instead
  // it is necessary to take a conservative approach.

  if (isa<AllocaInst>(I) || isa<GetElementPtrInst>(I) ||
      isa<StoreInst>(I) || isa<LoadInst>(I))
    return false;

  // llvm.gcroot is safe because it doesn't do anything at runtime.
  if (CallInst *CI = dyn_cast<CallInst>(I))
    if (Function *F = CI->getCalledFunction())
      if (unsigned IID = F->getIntrinsicID())
        if (IID == Intrinsic::gcroot)
          return false;

  return true;
}

/* We used LowerIntrinsics::InsertRootInitializers as a template. */
static bool customInsertRootInitializers(Function &F, AllocaInst **Roots,
                                         unsigned Count) {

  // Scroll past alloca instructions.
  BasicBlock::iterator IP = F.getEntryBlock().begin();
  while (isa<AllocaInst>(IP)) ++IP;

  // Search for initializers in the initial BB.
  SmallPtrSet<AllocaInst*,16> InitedRoots;
  for (; !CouldBecomeSafePoint(IP); ++IP)
    if (StoreInst *SI = dyn_cast<StoreInst>(IP))
      if (AllocaInst *AI =
          dyn_cast<AllocaInst>(SI->getOperand(1)->stripPointerCasts()))
        InitedRoots.insert(AI);

  // Add root initializers.
  bool MadeChange = false;

  for (AllocaInst **I = Roots, **E = Roots + Count; I != E; ++I)
    if (!InitedRoots.count(*I)) {
      // Store -5 (Erlang nil)
      StoreInst* SI = new StoreInst(ConstantInt::getSigned(
                          cast<PointerType>((*I)->getType())->getElementType(),
                          -5),*I);
      SI->insertAfter(*I);
      MadeChange = true;
    }

  return MadeChange;
}

/* Determine that the instruction is a Tail Call */
static bool isTailCall(MachineInstr *MI) {
  const MCInstrDesc& MCid = MI->getDesc();
  return (MCid.isCall() && MCid.isTerminator());

  // if (strcmp(InstrName,"TAILJMPd64") == 0 || // 64-bit
  //     strcmp(InstrName,"TAILJMPr64") == 0 ||
  //     strcmp(InstrName,"TAILJMPd") == 0   || // 32-bit
  //     strcmp(InstrName,"TAILJMPr") == 0)
  //   return true;
  // return false;
}

/* Copied from GCStrategy.cpp for findCustomSafePoints(...) */
static MCSymbol *InsertLabel(MachineBasicBlock &MBB,
                             MachineBasicBlock::iterator MI,
                             DebugLoc DL) {
  const TargetInstrInfo* TII = MBB.getParent()->getTarget().getInstrInfo();
  MCSymbol *Label = MBB.getParent()->getContext().CreateTempSymbol();
  BuildMI(MBB, MI, DL, TII->get(TargetOpcode::GC_LABEL)).addSym(Label);
  return Label;
}
//------------------------------------------------------------------------------

bool ErlangGC::performCustomLowering(Function &F) {
  SmallVector<AllocaInst*, 32> Roots;
  bool MadeChange = false;

  for (Function::iterator BB = F.begin(), E = F.end(); BB != E; ++BB)
    for (BasicBlock::iterator II = BB->begin(), E = BB->end(); II != E; )
      if (IntrinsicInst *CI = dyn_cast<IntrinsicInst>(II++))
        if (Function *F = CI->getCalledFunction())
          switch (F->getIntrinsicID()) {
          case Intrinsic::gcwrite:
            // Handle llvm.gcwrite.
            CI->eraseFromParent();
            MadeChange = true;
            break;
          case Intrinsic::gcread:
            // Handle llvm.gcread.
            CI->eraseFromParent();
            MadeChange = true;
            break;
          case Intrinsic::gcroot:
            // Handle llvm.gcroot.
            // Save the GC root for initialization, but do not delete
            // the intrinsic. The backend needs the intrinsic to flag
            // the stack slot.
            Roots.push_back(cast<AllocaInst>(
                            CI->getArgOperand(0)->stripPointerCasts()));
            MadeChange = true;
            break;
          default:
            continue;
          }

  // Perform custom root initialization (with a custom function!)
  if (Roots.size())
    MadeChange |= customInsertRootInitializers(F, Roots.begin(),
                                               Roots.size());

  return MadeChange;
}

bool ErlangGC::findCustomSafePoints(GCFunctionInfo &FI, MachineFunction &MF) {
  for (MachineFunction::iterator BBI = MF.begin(), BBE = MF.end();
       BBI != BBE; ++BBI)
    for (MachineBasicBlock::iterator MI = BBI->begin(), ME = BBI->end();
         MI != ME; ++MI)
      if (MI->getDesc().isCall() && !isTailCall(MI)) {
        /* Code copied from VisitCallPoint(...) */
        MachineBasicBlock::iterator RAI = MI; ++RAI;
        MCSymbol* Label = InsertLabel(*MI->getParent(), RAI, MI->getDebugLoc());
        FI.addSafePoint(GC::PostCall, Label, MI->getDebugLoc());
      }
  return false;
}
