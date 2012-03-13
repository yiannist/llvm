; RUN: llc < %s -mcpu=generic -mtriple=i686-linux -enable-hipe-prologue -verify-machineinstrs | FileCheck %s -check-prefix=X32-Linux
; RUN: llc < %s -mtriple=x86_64-linux-gnu  -enable-hipe-prologue -verify-machineinstrs | FileCheck %s -check-prefix=X64-Linux

; Just to prevent the alloca from being optimized away
declare void @dummy_use(i32*, i32)

define {i32, i32} @test_basic(i32 %hp, i32 %p) {
  ; X32-Linux:       test_basic:
  ; X32-Linux-NOT:   calll inc_stack_0

  ; X64-Linux:       test_basic
  ; X64-Linux-NOT:   callq inc_stack_0

  %mem = alloca i32, i32 10
  call void @dummy_use (i32* %mem, i32 10)
  %1 = insertvalue {i32, i32} undef, i32 %hp, 0
  %2 = insertvalue {i32, i32} %1, i32 %p, 1
  ret {i32, i32} %1
}

define cc 11 {i32, i32} @test_basic_hipecc(i32 %hp, i32 %p) {
  ; X32-Linux:       test_basic_hipecc:
  ; X32-Linux:       jmp .LBB1_1

  ; X32-Linux:       .LBB1_2:
  ; X32-Linux-NEXT:  calll inc_stack_0

  ; X32-Linux:       .LBB1_1:
  ; X32-Linux-NEXT:  leal -164(%esp), %ebx
  ; X32-Linux-NEXT:  cmpl 36(%ebp), %ebx
  ; X32-Linux-NEXT:  jb .LBB1_2

  ; X32-Linux:       subl $60, %esp

  ; X64-Linux:       test_basic_hipecc:
  ; X64-Linux:       jmp .LBB1_1

  ; X64-Linux:       .LBB1_2:
  ; X64-Linux-NEXT:  callq inc_stack_0

  ; X64-Linux:       .LBB1_1:
  ; X64-Linux-NEXT:  leaq -248(%rsp), %r14
  ; X64-Linux-NEXT:  cmpq 72(%rbp), %r14
  ; X64-Linux-NEXT:  jb .LBB1_2

  ; X64-Linux:       subq $40, %rsp

  %mem = alloca i32, i32 10
  call void @dummy_use (i32* %mem, i32 10)
  %1 = insertvalue {i32, i32} undef, i32 %hp, 0
  %2 = insertvalue {i32, i32} %1, i32 %p, 1
  ret {i32, i32} %2
}

define cc 11 {i32,i32,i32} @test_nocall_hipecc(i32 %hp,i32 %p,i32 %x,i32 %y) {
  ; X32-Linux:       test_nocall_hipecc:
  ; X32-Linux-NOT:   callq inc_stack_0

  ; X64-Linux:       test_nocall_hipecc:
  ; X64-Linux-NOT:   callq inc_stack_0

  %1 = add i32 %x, %y
  %2 = mul i32 42, %1
  %3 = sub i32 24, %2
  %4 = insertvalue {i32, i32, i32} undef, i32 %hp, 0
  %5 = insertvalue {i32, i32, i32} %4, i32 %p, 1
  %6 = insertvalue {i32, i32, i32} %5, i32 %p, 2
  ret {i32, i32, i32} %6
}

