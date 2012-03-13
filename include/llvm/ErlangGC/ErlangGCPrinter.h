// lib/ErlangGCPrinter.h

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

