//===-- llvm/Erlang.h - Useful defines for Erlang/OTP backend  --*- C++ -*-===//
//
//===----------------------------------------------------------------------===//
//
/// @file
/// This file contains some very handful macros and variables already defined in
/// Erlang/OTP ABI that are needed in ErlangGC plugin and custom emitPrologue
/// for appropriate (i.e. that confronts to the ABI) Stack management. (see
/// otp/lib/hipe/x86/hipe_x86_frame.erl)
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_ERLANG_H
#define LLVM_ERLANG_H

/* AMD64_LEAF_WORDS */
const int LeafWords = 24;
/* HIPE_X86_REGISTERS */
int WordSize(bool Is64Bit) {
    return Is64Bit ? 8 : 4;
}
/* First NrArgs arguments to be saved in registers (not stack).
 * HIPE_{X86,AMD64}_REGISTERS:nr_args() */
int NrArgs(bool Is64Bit) {
    return Is64Bit ? 4 : 3;
}
/* Pinned Virtual registers (HP, P etc.). Notice: NSP removed from cc.*/
const int NrPinnedRegs = 2;
/* Total registers used */
int TotalRegs(bool Is64Bit) {
    return NrArgs(Is64Bit) + NrPinnedRegs;
}
/* Stack arity is the number of _stacked-only_ arguments */
int computeStkArity(int arg_size, bool Is64Bit) {
    return arg_size > TotalRegs(Is64Bit) ? arg_size - TotalRegs(Is64Bit) : 0;
}

#endif
