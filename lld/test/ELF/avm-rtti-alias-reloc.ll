; RUN: split-file %s %t
; RUN: llc -mtriple=avm-unknown-arduboyfx -filetype=obj %t/start.ll -o %t/start.o
; RUN: llc -mtriple=avm-unknown-arduboyfx -filetype=obj %t/rtti.ll -o %t/rtti.o
; RUN: llc -mtriple=avm-unknown-arduboyfx -filetype=obj %t/nomarker.ll -o %t/nomarker.o

; A constant typeid initializer is a 16-bit data reference to a program object. The
; marker lets LLD accept it, and the stored value is the object's AS0 alias.
; RUN: ld.lld %t/start.o %t/rtti.o -o %t/rtti.elf
; RUN: llvm-readobj --sections --symbols %t/rtti.elf | FileCheck %s --check-prefix=ALIAS
; RUN: llvm-objdump -s -j .data %t/rtti.elf | FileCheck %s --check-prefix=BYTES

; Without a marker, a 16-bit reference to a program object stays invalid.
; RUN: not ld.lld %t/start.o %t/nomarker.o -o %t/nomarker.elf 2>&1 \
; RUN:   | FileCheck %s --check-prefix=NOMARKER

; ALIAS:      Name: .avm.rtti
; ALIAS:      Address: 0x500
; ALIAS:      Name: _ZTI1A
; ALIAS-NEXT: Value: 0x500

; BYTES: Contents of section .data:
; BYTES-NEXT: 0100 000b

; NOMARKER: error: AVM relocation R_AVM_DATA16 against symbol 'typeinfo for A' requires a data-space symbol

;--- start.ll
target datalayout = "e-m:e-p:16:8-p1:24:8-i8:8-i16:8-i32:8-i64:8-f16:8-f32:8-n8:16-S8-P1-G0-A0"
target triple = "avm-unknown-arduboyfx"

define void @_start() {
  ret void
}

;--- rtti.ll
target datalayout = "e-m:e-p:16:8-p1:24:8-i8:8-i16:8-i32:8-i64:8-f16:8-f32:8-n8:16-S8-P1-G0-A0"
target triple = "avm-unknown-arduboyfx"

module asm ".weak __avm_rtti_alias._ZTI1A"

@_ZTI1A = addrspace(1) constant [6 x i8] zeroinitializer, section ".avm.rtti.type._ZTI1A", align 1
@use = global ptr inttoptr (i16 trunc (i32 add (i32 ptrtoint (ptr addrspace(1) @_ZTI1A to i32), i32 1536) to i16) to ptr), align 1

;--- nomarker.ll
target datalayout = "e-m:e-p:16:8-p1:24:8-i8:8-i16:8-i32:8-i64:8-f16:8-f32:8-n8:16-S8-P1-G0-A0"
target triple = "avm-unknown-arduboyfx"

@_ZTI1A = addrspace(1) constant [6 x i8] zeroinitializer, section ".avm.rtti.type._ZTI1A", align 1
@use = global ptr inttoptr (i16 trunc (i32 add (i32 ptrtoint (ptr addrspace(1) @_ZTI1A to i32), i32 1536) to i16) to ptr), align 1
