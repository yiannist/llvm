//=== ErlangGCPrinter.h - A GC printer for HiPE compiler Impl ---*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file describes the implementation of and registers the Erlang garbage
// collector.
//
//===----------------------------------------------------------------------===//

#include "llvm/CodeGen/GCMetadataPrinter.h"
#include "llvm/Support/Compiler.h"

using namespace llvm;

namespace {
    class LLVM_LIBRARY_VISIBILITY ErlangGCPrinter : public GCMetadataPrinter {
    public:
      virtual void beginAssembly(AsmPrinter &AP);
      virtual void finishAssembly(AsmPrinter &AP);
    };

    GCMetadataPrinterRegistry::Add<ErlangGCPrinter>
    X("erlang_gc", "Erlang's garbage collector printer.");
}
