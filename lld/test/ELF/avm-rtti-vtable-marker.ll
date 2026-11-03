; RUN: split-file %s %t
; RUN: llc -mtriple=avm-unknown-arduboyfx -filetype=obj %t/start.ll -o %t/start.o
; RUN: llc -mtriple=avm-unknown-arduboyfx -filetype=obj %t/vtable.ll -o %t/vtable.o
; RUN: llc -mtriple=avm-unknown-arduboyfx -filetype=obj %t/high.ll -o %t/high.o

; The module has no typeid. Its vtable names the descriptor, and the vtable's marker is the
; only reference to the window check. The vtable itself lies above 0xFFFF, in .rodata, while
; its descriptor lies in the flash window, so the link is valid when the window copy is kept.
; RUN: ld.lld --image-base=0x100 --section-start=.data=0x100 \
; RUN:   --section-start=.avm.rtti=0x600 --section-start=.text=0x2000 \
; RUN:   --section-start=.rodata=0x10000 %t/start.o %t/vtable.o %t/high.o -o %t/window.elf
; RUN: llvm-readobj --symbols %t/window.elf | FileCheck %s --check-prefix=WINDOW

; When the high copy of the descriptor is kept, the vtable's marker rejects the link, even
; though no module queries typeid.
; RUN: not ld.lld --image-base=0x100 --section-start=.data=0x100 \
; RUN:   --section-start=.avm.rtti=0x600 --section-start=.text=0x2000 \
; RUN:   --section-start=.rodata=0x10000 %t/start.o %t/high.o %t/vtable.o -o %t/high-first.elf 2>&1 \
; RUN:   | FileCheck %s --check-prefix=HIGH

; WINDOW:      Name: _ZTI1A
; WINDOW-NEXT: Value: 0x600
; WINDOW:      Name: _ZTV1A
; WINDOW-NEXT: Value: 0x10000

; HIGH: error: AVM type_info descriptor '_ZTI1A' is not addressable through the AS0 flash window

;--- start.ll
target datalayout = "e-m:e-p:16:8-p1:24:8-i8:8-i16:8-i32:8-i64:8-f16:8-f32:8-n8:16-S8-P1-G0-A0"
target triple = "avm-unknown-arduboyfx"

define void @_start() {
  ret void
}

;--- vtable.ll
target datalayout = "e-m:e-p:16:8-p1:24:8-i8:8-i16:8-i32:8-i64:8-f16:8-f32:8-n8:16-S8-P1-G0-A0"
target triple = "avm-unknown-arduboyfx"

; The vtable's address point is six bytes past the symbol. Its RTTI entry names the
; descriptor, and the weak marker is what clang emits for each vtable's descriptor.
module asm ".weak __avm_rtti_alias._ZTI1A"

$_ZTI1A = comdat any
@_ZTI1A = linkonce_odr addrspace(1) constant [6 x i8] zeroinitializer, section ".avm.rtti.type._ZTI1A", comdat, align 1
@_ZTV1A = dso_local unnamed_addr addrspace(1) constant [3 x ptr addrspace(1)] [ptr addrspace(1) null, ptr addrspace(1) @_ZTI1A, ptr addrspace(1) null], align 1

;--- high.ll
target datalayout = "e-m:e-p:16:8-p1:24:8-i8:8-i16:8-i32:8-i64:8-f16:8-f32:8-n8:16-S8-P1-G0-A0"
target triple = "avm-unknown-arduboyfx"

$_ZTI1A = comdat any
@_ZTI1A = linkonce_odr addrspace(1) constant [6 x i8] zeroinitializer, comdat, align 1
