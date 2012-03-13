; RUN: llc < %s -tailcallopt -mtriple=x86_64-linux-gnu | FileCheck %s

; Check the HiPE calling convention works (x86-64)

@hp   = external global i64 ; assigned to register: R15
@p    = external global i64 ; assigned to register: RBP
@arg0 = external global i64 ; assigned to register: RSI
@arg1 = external global i64 ; assigned to register: RDX
@arg2 = external global i64 ; assigned to register: RCX
@arg3 = external global i64 ; assigned to register: R8

define void @zap(i64 %a, i64 %b) nounwind {
entry:
  ; CHECK:      movq %rdi, %r15
  ; CHECK-NEXT: movq %rsi, %rbp
  ; CHECK-NEXT: callq addtwo
  %0 = call cc 11 i64 @addtwo(i64 %a, i64 %b)
  ; CHECK:      callq foo
  call void @foo() nounwind
  ret void
}

define cc 11 i64 @addtwo(i64 %x, i64 %y) nounwind {
entry:
  ; CHECK:      addq %rbp, %r15
  %0 = add i64 %x, %y
  ; CHECK-NEXT: ret
  ret i64 %0
}

define cc 11 void @foo() nounwind {
entry:
  ; CHECK:      movq hp(%rip), %r15
  ; CHECK-NEXT: movq p(%rip), %rbp
  ; CHECK-NEXT: movq arg0(%rip), %rsi
  ; CHECK-NEXT: movq arg1(%rip), %rdx
  ; CHECK-NEXT: movq arg2(%rip), %rcx
  ; CHECK-NEXT: movq arg3(%rip), %r8
  %0 = load i64* @arg3
  %1 = load i64* @arg2
  %2 = load i64* @arg1
  %3 = load i64* @arg0
  %4 = load i64* @p
  %5 = load i64* @hp
  ; CHECK: jmp bar
  tail call cc 11 void @bar( i64 %5, i64 %4, i64 %3, i64 %2, i64 %1,
                             i64 %0 ) nounwind
  ret void
}

declare cc 11 void @bar(i64, i64, i64, i64, i64, i64)
