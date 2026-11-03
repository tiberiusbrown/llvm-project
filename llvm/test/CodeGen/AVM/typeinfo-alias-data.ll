; RUN: llc -mtriple=avm-unknown-arduboyfx < %s | FileCheck %s

; A typeid alias in a static initializer is the 16-bit AS0 address of a program-space
; RTTI object. The AsmPrinter names the object with its program address plus the
; 0x0600 window bias, and the linker checks the object's placement.

target datalayout = "e-m:e-p:16:8-p1:24:8-i8:8-i16:8-i32:8-i64:8-f16:8-f32:8-n8:16-S8-P1-G0-A0"
target triple = "avm-unknown-arduboyfx"

@_ZTI1A = addrspace(1) constant [6 x i8] zeroinitializer, section ".avm.rtti.type._ZTI1A", align 1

@alias = constant ptr inttoptr (i16 trunc (i32 add (i32 ptrtoint (ptr addrspace(1) @_ZTI1A to i32), i32 1536) to i16) to ptr), align 1

@pair = constant { ptr, ptr } { ptr inttoptr (i16 trunc (i32 add (i32 ptrtoint (ptr addrspace(1) @_ZTI1A to i32), i32 1536) to i16) to ptr), ptr null }, align 1

; CHECK-LABEL: alias:
; CHECK-NEXT:  .short _ZTI1A+1536
; CHECK-LABEL: pair:
; CHECK-NEXT:  .short _ZTI1A+1536
; CHECK-NEXT:  .short 0
