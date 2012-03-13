; RUN: llc < %s -tailcallopt -mtriple=i686-linux-gnu | FileCheck %s

; Test the HiPE calling convention works (x86-32)

@hp   = external global i32 ; assigned to register: ESI
@p    = external global i32 ; assigned to register: EBP
@arg0 = external global i32 ; assigned to register: EAX
@arg1 = external global i32 ; assigned to register: EDX
@arg2 = external global i32 ; assigned to register: ECX

define void @zap(i32 %a, i32 %b) nounwind {
entry:
  ; CHECK:      movl {{[0-9]*}}(%esp), %esi
  ; CHECK-NEXT: movl {{[0-9]*}}(%esp), %ebp
  ; CHECK-NEXT: calll addtwo
  %0 = call cc 11 i32 @addtwo(i32 %a, i32 %b)
  ; CHECK: calll foo
  call void @foo() nounwind
  ret void
}

define cc 11 i32 @addtwo(i32 %x, i32 %y) nounwind {
entry:
  ; CHECK:      addl %ebp, %esi
  %0 = add i32 %x, %y
  ; CHECK-NEXT: ret
  ret i32 %0
}

define cc 11 void @foo() nounwind {
entry:
  ; CHECK:      movl hp, %esi
  ; CHECK-NEXT: movl p, %ebp
  ; CHECK-NEXT: movl arg0, %eax
  ; CHECK-NEXT: movl arg1, %edx
  ; CHECK-NEXT: movl arg2, %ecx
  %0 = load i32* @arg2
  %1 = load i32* @arg1
  %2 = load i32* @arg0
  %3 = load i32* @p
  %4 = load i32* @hp
  ; CHECK: jmp bar
  tail call cc 11 void @bar( i32 %4, i32 %3, i32 %2, i32 %1, i32 %0 ) nounwind
  ret void
}

declare cc 11 void @bar(i32, i32, i32, i32, i32)
