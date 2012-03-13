// lib/ErlangGC/ErlangGCPrinter.cpp - An LLVM GC printer for HiPE

#include "llvm/CodeGen/AsmPrinter.h"
#include "llvm/Function.h"
#include "llvm/Target/TargetMachine.h"
#include "llvm/Target/TargetData.h"
#include "llvm/MC/MCAsmInfo.h"
#include "llvm/MC/MCStreamer.h"
#include "llvm/MC/MCSymbol.h"
#include "llvm/MC/MCSectionELF.h"
#include "llvm/MC/MCContext.h"
#include "llvm/ErlangGC/ErlangGCPrinter.h"
#include "llvm/Erlang.h" // For computeStkArity(...)

void ErlangGCPrinter::beginAssembly(AsmPrinter &AP) {
  // Nothing to do.
}

void ErlangGCPrinter::finishAssembly(AsmPrinter &AP) {
  const MCAsmInfo *TAI = AP.MAI;
  MCStreamer &OS = AP.OutStreamer;
  bool Is64Bit = AP.TM.getTargetData()->getPointerSize() == sizeof(int64_t);

  // Set up for emitting addresses.
  const char *AddressDirective;
  int AddressAlignLog;
  if (Is64Bit) {
    AddressDirective = TAI->getData64bitsDirective();
    AddressAlignLog = 3;
  } else {
    AddressDirective = TAI->getData32bitsDirective();
    AddressAlignLog = 2;
  }

  // Put this in a custom note section. (XXX: Is this the correct Kind??)
  OS.SwitchSection(OS.getContext().getELFSection(".note.gc"/*Section*/,
                                                 ELF::SHT_PROGBITS/*Type*/,
                                                 0/*Flags*/,
                                                 SectionKind::getDataRel()/*Kind*/));
  AP.getCurrentSection()->PrintSwitchToSection(*AP.MAI, OS.GetCommentOS());

  // For each function...
  for (iterator FI = begin(), FE = end(); FI != FE; ++FI) {
    GCFunctionInfo &MD = **FI;

    /** A more compact GC layout.
     ** Emit this data structure:
     *
     * struct {
     *	 int32_t PointCount;
     *	 void *SafePointAddress[PointCount];
     *	 uint64_t StackFrameSize; (in words)
     *	 size_t StackArity;
     *	 int32_t LiveCount;
     *   int32_t LiveOffsets[LiveCount];
     * } __gcmap_<FUNCTIONNAME>;
     **/

    // Emit the symbol by which the stack map entry can be found.
    std::string Symbol;
    Symbol += TAI->getGlobalPrefix();
    Symbol += "__gcmap_";
    Symbol += MD.getFunction().getName();
    // if (const char *GlobalDirective = TAI->getGlobalDirective())
    //     OS.GetCommentOS() << GlobalDirective << Symbol << "\n";
    OS.GetCommentOS() << TAI->getGlobalPrefix() << Symbol << ":\n";

    // Align to address width.
    AP.EmitAlignment(AddressAlignLog);

    // Emit PointCount.
    OS.AddComment("safe point count");
    AP.EmitInt32(MD.size());

    // And each safe point...
    for (GCFunctionInfo::iterator PI = MD.begin(),
           PE = MD.end(); PI != PE; ++PI) {
      // Align to address width.
      //AP.EmitAlignment(AddressAlignLog);

      // Emit the address of the safe point.
      MCSymbol *Label = PI->Label;
      OS.GetCommentOS() << "safe point address: ";
      OS.GetCommentOS() << AddressDirective << TAI->getPrivateGlobalPrefix()
                        << "label " << Label->getName() << "\n";
      AP.EmitLabelPlusOffset(Label/*Hi*/, 0/*Offset*/, 4/*Size*/);
    }

    // Stack information never change in safe points!
    // Just print info from the first call site (they don't change, anyway).
    GCFunctionInfo::iterator PI = MD.begin();

    // Emit the stack frame size.
    OS.AddComment("stack frame size (in words)");
    const unsigned int WordSz = WordSize(Is64Bit);
    AP.EmitInt32(MD.getFrameSize() / WordSz);

    // Emit StkArity
    OS.AddComment("stack arity");
    /* Stack arity is the number of _stacked-only_ arguments */
    const Function &Fn = MD.getFunction();
    const unsigned int StkArity = computeStkArity(Fn.arg_size(), Is64Bit);
    AP.EmitInt32(StkArity);

    // Emit the number of live roots in the function.
    OS.AddComment("live root count");
    AP.EmitInt32(MD.live_size(PI));

    // And for each live root...
    for (GCFunctionInfo::live_iterator LI = MD.live_begin(PI),
           LE = MD.live_end(PI); LI != LE; ++LI) {
      // Emit live root's offset within the stack frame.
      OS.AddComment("stack index (offset / wordsize)");
      AP.EmitInt32(LI->StackOffset / WordSz);
      //OS.EmitRawText(Twine("\t.long\t-5\t"));
    }
  }
}
