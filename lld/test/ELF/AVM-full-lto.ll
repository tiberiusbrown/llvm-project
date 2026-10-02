; RUN: split-file %s %t
; RUN: llvm-as %t/entry.ll -o %t/entry.o
; RUN: llvm-as %t/helper.ll -o %t/helper.o
; RUN: llvm-ar rcs %t/libhelper.a %t/helper.o
; RUN: llvm-mc -triple=avm -filetype=obj %t/data.s -o %t/data.o
; RUN: ld.lld --gc-sections -e _start %t/entry.o %t/data.o %t/libhelper.a -o %t/a.out
; RUN: llvm-readobj --file-headers --sections --symbols %t/a.out | FileCheck %s --check-prefix=ELF --implicit-check-not='Name: helper'
; RUN: llvm-objdump -d %t/a.out | FileCheck %s --check-prefix=CODE --implicit-check-not=call

; Check regular LTO across two AVM bitcode modules, including a lazy archive
; member and a native data-space object. The helper call must fold to 14.

; ELF: Format: elf32-avm
; ELF: Machine: 0x4156
; ELF: Name: .data
; ELF: SHF_AVM_DATASPACE
; ELF: Name: .text
; ELF: SHF_AVM_PROGSPACE
; ELF: Name: _start
; ELF: Name: sink

; CODE: <_start>:
; CODE: ldi8{{[ \t]+}}r{{[0-7]}}, 0xe
; CODE: stm16

;--- entry.ll
target datalayout = "e-m:e-p:16:8-p1:24:8-i8:8-i16:8-i32:8-i64:8-f16:8-f32:8-n8:16-S8-P1-G0-A0"
target triple = "avm-unknown-arduboyfx"

@sink = external global i16
declare i16 @helper(i16) addrspace(1)

define void @_start() addrspace(1) {
entry:
  %value = call addrspace(1) i16 @helper(i16 7)
  store volatile i16 %value, ptr @sink
  br label %loop

loop:
  br label %loop
}

;--- helper.ll
target datalayout = "e-m:e-p:16:8-p1:24:8-i8:8-i16:8-i32:8-i64:8-f16:8-f32:8-n8:16-S8-P1-G0-A0"
target triple = "avm-unknown-arduboyfx"

define i16 @helper(i16 %x) addrspace(1) {
  %result = add i16 %x, %x
  ret i16 %result
}

;--- data.s
.section .data.sink,"aw",@progbits
.globl sink
sink:
  .short 0
