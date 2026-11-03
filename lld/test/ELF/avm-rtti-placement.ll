; RUN: split-file %s %t
; RUN: llc -mtriple=avm-unknown-arduboyfx -filetype=obj %t/start.ll -o %t/start.o
; RUN: llc -mtriple=avm-unknown-arduboyfx -filetype=obj %t/flash.ll -o %t/flash.o
; RUN: llc -mtriple=avm-unknown-arduboyfx -filetype=obj %t/rtti.ll -o %t/rtti.o
; RUN: llc -mtriple=avm-unknown-arduboyfx -filetype=obj %t/high.ll -o %t/high.o

; The RTTI objects follow the flash image bytes, and .text follows the RTTI bytes.
; RUN: ld.lld %t/start.o %t/flash.o %t/rtti.o -o %t/rtti.elf
; RUN: llvm-readobj --sections --symbols %t/rtti.elf | FileCheck %s --check-prefix=PLACED

; The section must end inside the window, so a section that starts too late overflows.
; RUN: not ld.lld %t/start.o %t/rtti.o --image-base=0x100 --section-start=.data=0x100 \
; RUN:   --section-start=.text=0x2000 --section-start=.avm.rtti=0xF9FA -o %t/over.elf 2>&1 \
; RUN:   | FileCheck %s --check-prefix=OVERFLOW

; An unreferenced descriptor is discarded before the window check, so it does not overflow.
; RUN: ld.lld --gc-sections %t/start.o %t/rtti.o --image-base=0x100 --section-start=.data=0x100 \
; RUN:   --section-start=.text=0x2000 --section-start=.avm.rtti=0xF9FA -o %t/gc.elf
; RUN: llvm-readobj --sections --symbols %t/gc.elf | FileCheck %s --check-prefix=GC

; The section cannot start below the first AS0-addressable program byte.
; RUN: not ld.lld %t/start.o %t/rtti.o --image-base=0x100 --section-start=.data=0x100 \
; RUN:   --section-start=.text=0x2000 --section-start=.avm.rtti=0x400 -o %t/low.elf 2>&1 \
; RUN:   | FileCheck %s --check-prefix=LOW

; A copy that a marker names must have a window address, so a -fno-rtti copy that
; a higher section keeps above the window is rejected.
; RUN: not ld.lld %t/start.o %t/high.o --image-base=0x100 --section-start=.data=0x100 \
; RUN:   --section-start=.text=0x2000 --section-start=.rodata=0x10000 \
; RUN:   -o %t/high.elf 2>&1 | FileCheck %s --check-prefix=HIGH

; PLACED:      Name: .flash
; PLACED:      Address: 0xB00
; PLACED:      Size: 4
; PLACED:      Name: .avm.rtti
; PLACED:      Flags [
; PLACED-NEXT:   SHF_ALLOC
; PLACED-NEXT:   SHF_AVM_PROGSPACE
; PLACED-NEXT: ]
; PLACED:      Address: 0x504
; PLACED:      Size: 9
; PLACED:      Name: .text
; PLACED:      Address: 0x50D
; PLACED:      Name: _ZTI1A
; PLACED-NEXT: Value: 0x504

; OVERFLOW: error: AVM RTTI flash-window overflow:
; OVERFLOW: RTTI section ends at program address 0x00FA03,
; OVERFLOW: but the last addressable byte is 0x00F9FF

; GC-NOT: Name: .avm.rtti
; GC-NOT: _ZTI1A

; LOW: error: AVM .avm.rtti must begin at program address 0x0500 or later
; LOW: error: AVM type_info descriptor '_ZTI1A' is not addressable through the AS0 flash window

; HIGH: error: AVM type_info descriptor '_ZTI1A' is not addressable through the AS0 flash window

;--- start.ll
target datalayout = "e-m:e-p:16:8-p1:24:8-i8:8-i16:8-i32:8-i64:8-f16:8-f32:8-n8:16-S8-P1-G0-A0"
target triple = "avm-unknown-arduboyfx"

define void @_start() {
  ret void
}

;--- flash.ll
target datalayout = "e-m:e-p:16:8-p1:24:8-i8:8-i16:8-i32:8-i64:8-f16:8-f32:8-n8:16-S8-P1-G0-A0"
target triple = "avm-unknown-arduboyfx"

@flash_bytes = constant [4 x i8] c"\01\02\03\04", align 1

;--- rtti.ll
target datalayout = "e-m:e-p:16:8-p1:24:8-i8:8-i16:8-i32:8-i64:8-f16:8-f32:8-n8:16-S8-P1-G0-A0"
target triple = "avm-unknown-arduboyfx"

module asm ".weak __avm_rtti_alias._ZTI1A"

@_ZTI1A = addrspace(1) constant [6 x i8] zeroinitializer, section ".avm.rtti.type._ZTI1A", align 1
@_ZTS1A = addrspace(1) constant [3 x i8] c"1A\00", section ".avm.rtti.name._ZTS1A", align 1

;--- high.ll
target datalayout = "e-m:e-p:16:8-p1:24:8-i8:8-i16:8-i32:8-i64:8-f16:8-f32:8-n8:16-S8-P1-G0-A0"
target triple = "avm-unknown-arduboyfx"

module asm ".weak __avm_rtti_alias._ZTI1A"

@_ZTI1A = addrspace(1) constant [6 x i8] zeroinitializer, section ".rodata._ZTI1A", align 1
