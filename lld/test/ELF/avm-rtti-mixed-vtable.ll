; RUN: split-file %s %t
; RUN: llc -mtriple=avm-unknown-arduboyfx -filetype=obj %t/start.ll -o %t/start.o
; RUN: llc -mtriple=avm-unknown-arduboyfx -filetype=obj %t/start_nohelper.ll -o %t/start_nohelper.o
; RUN: llc -mtriple=avm-unknown-arduboyfx -filetype=obj %t/start_marker.ll -o %t/start_marker.o
; RUN: llc -mtriple=avm-unknown-arduboyfx -filetype=obj %t/helper.ll -o %t/helper.o
; RUN: llc -mtriple=avm-unknown-arduboyfx -filetype=obj %t/nortti.ll -o %t/nortti.o
; RUN: llc -mtriple=avm-unknown-arduboyfx -filetype=obj %t/nortti_dead.ll -o %t/nortti_dead.o
; RUN: llc -mtriple=avm-unknown-arduboyfx -filetype=obj %t/rtti_window.ll -o %t/rtti_window.o
; RUN: llc -mtriple=avm-unknown-arduboyfx -filetype=obj %t/rtti.ll -o %t/rtti.o
; RUN: llc -mtriple=avm-unknown-arduboyfx -filetype=obj %t/desc.ll -o %t/desc.o
; RUN: llc -mtriple=avm-unknown-arduboyfx -filetype=obj %t/nortti_c.ll -o %t/nortti_c.o
; RUN: llc -mtriple=avm-unknown-arduboyfx -filetype=obj %t/rtti_c.ll -o %t/rtti_c.o
; RUN: llc -mtriple=avm-unknown-arduboyfx -filetype=obj %t/rtti_comdat.ll -o %t/rtti_comdat.o
; RUN: llc -mtriple=avm-unknown-arduboyfx -filetype=obj %t/nortti_comdat.ll -o %t/nortti_comdat.o
; RUN: llc -mtriple=avm-unknown-arduboyfx -filetype=obj %t/weak_rtti.ll -o %t/weak_rtti.o
; RUN: llc -mtriple=avm-unknown-arduboyfx -filetype=obj %t/zero_rtti.ll -o %t/zero_rtti.o
; RUN: llc -mtriple=avm-unknown-arduboyfx -filetype=obj %t/placeholder.ll -o %t/placeholder.o
; RUN: llc -mtriple=avm-unknown-arduboyfx -filetype=obj %t/type_info.ll -o %t/type_info.o
; RUN: llc -mtriple=avm-unknown-arduboyfx -filetype=obj %t/local_nortti.ll -o %t/local_nortti.o
; RUN: llc -mtriple=avm-unknown-arduboyfx -filetype=obj %t/local_rtti.ll -o %t/local_rtti.o
; RUN: llc -mtriple=avm-unknown-arduboyfx -filetype=obj %t/shared.ll -o %t/shared.o
; RUN: llc -mtriple=avm-unknown-arduboyfx -filetype=obj %t/shared_rev.ll -o %t/shared_rev.o

; The layout matches the other high-address RTTI fixtures. Descriptors and window vtables
; go in .avm.rtti at 0x0600, and the other program-space data goes to 0x10000.

; 1. A live helper and an RTTI-disabled complete-object vtable are rejected.
; RUN: not ld.lld --image-base=0x100 --section-start=.data=0x100 --section-start=.avm.rtti=0x600 --section-start=.text=0x2000 --section-start=.rodata=0x10000 %t/start.o %t/helper.o %t/nortti.o -o %t/1.elf 2>&1 | FileCheck %s --check-prefix=NORTTI-V

; 2. A live helper and an RTTI-enabled vtable placed in the flash window are accepted.
; RUN: ld.lld --image-base=0x100 --section-start=.data=0x100 --section-start=.avm.rtti=0x600 --section-start=.text=0x2000 --section-start=.rodata=0x10000 %t/start.o %t/helper.o %t/rtti_window.o %t/desc.o -o %t/2.elf
; RUN: llvm-readobj --symbols %t/2.elf | FileCheck %s --check-prefix=WINDOW-V

; 3. A live helper and an RTTI-disabled construction vtable are rejected.
; RUN: not ld.lld --image-base=0x100 --section-start=.data=0x100 --section-start=.avm.rtti=0x600 --section-start=.text=0x2000 --section-start=.rodata=0x10000 %t/start.o %t/helper.o %t/nortti_c.o -o %t/3.elf 2>&1 | FileCheck %s --check-prefix=NORTTI-C

; 4. A live helper and an RTTI-enabled construction vtable are accepted.
; RUN: ld.lld --image-base=0x100 --section-start=.data=0x100 --section-start=.avm.rtti=0x600 --section-start=.text=0x2000 --section-start=.rodata=0x10000 %t/start.o %t/helper.o %t/rtti_c.o %t/desc.o -o %t/4.elf

; 5. A dead RTTI-disabled vtable does not activate the check under --gc-sections.
; RUN: ld.lld --gc-sections --image-base=0x100 --section-start=.data=0x100 --section-start=.avm.rtti=0x600 --section-start=.text=0x2000 --section-start=.rodata=0x10000 %t/start.o %t/helper.o %t/nortti_dead.o -o %t/5.elf

; 6. A discarded RTTI-disabled COMDAT copy does not affect a selected RTTI-enabled copy.
; RUN: ld.lld --image-base=0x100 --section-start=.data=0x100 --section-start=.avm.rtti=0x600 --section-start=.text=0x2000 --section-start=.rodata=0x10000 %t/start.o %t/helper.o %t/rtti_comdat.o %t/nortti_comdat.o -o %t/6.elf

; 7. A discarded RTTI-enabled COMDAT copy does not hide a selected RTTI-disabled copy.
; RUN: not ld.lld --image-base=0x100 --section-start=.data=0x100 --section-start=.avm.rtti=0x600 --section-start=.text=0x2000 --section-start=.rodata=0x10000 %t/start.o %t/helper.o %t/nortti_comdat.o %t/rtti_comdat.o -o %t/7.elf 2>&1 | FileCheck %s --check-prefix=COMDAT-LOSER

; 8. Without a live helper, an RTTI-disabled vtable is accepted.
; RUN: ld.lld --image-base=0x100 --section-start=.data=0x100 --section-start=.avm.rtti=0x600 --section-start=.text=0x2000 --section-start=.rodata=0x10000 %t/start_nohelper.o %t/nortti.o -o %t/8.elf

; 9. A static typeid alias marker alone does not activate the check.
; RUN: ld.lld --image-base=0x100 --section-start=.data=0x100 --section-start=.avm.rtti=0x600 --section-start=.text=0x2000 --section-start=.rodata=0x10000 %t/start_marker.o %t/desc.o %t/nortti.o -o %t/9.elf

; 10. A high-address vtable with a window-resident RTTI descriptor is accepted.
; RUN: ld.lld --image-base=0x100 --section-start=.data=0x100 --section-start=.avm.rtti=0x600 --section-start=.text=0x2000 --section-start=.rodata=0x10000 %t/start.o %t/helper.o %t/rtti.o %t/desc.o -o %t/10.elf

; 11. An undefined weak RTTI target, which resolves to zero, is rejected.
; RUN: not ld.lld --image-base=0x100 --section-start=.data=0x100 --section-start=.avm.rtti=0x600 --section-start=.text=0x2000 --section-start=.rodata=0x10000 %t/start.o %t/helper.o %t/weak_rtti.o -o %t/11.elf 2>&1 | FileCheck %s --check-prefix=INVALID-TARGET

; 11b. A zero-size descriptor is not a valid RTTI target.
; RUN: not ld.lld --image-base=0x100 --section-start=.data=0x100 --section-start=.avm.rtti=0x600 --section-start=.text=0x2000 --section-start=.rodata=0x10000 %t/start.o %t/helper.o %t/zero_rtti.o -o %t/11b.elf 2>&1 | FileCheck %s --check-prefix=INVALID-TARGET

; 12. The runtime's RTTI support-vtable placeholders are excluded.
; RUN: ld.lld --image-base=0x100 --section-start=.data=0x100 --section-start=.avm.rtti=0x600 --section-start=.text=0x2000 --section-start=.rodata=0x10000 %t/start.o %t/helper.o %t/placeholder.o -o %t/12.elf

; 12b. The runtime's std::type_info vtable has no RTTI slot. No object can have that exact
; dynamic type, so the check does not apply to it.
; RUN: ld.lld --image-base=0x100 --section-start=.data=0x100 --section-start=.avm.rtti=0x600 --section-start=.text=0x2000 --section-start=.rodata=0x10000 %t/start.o %t/helper.o %t/type_info.o -o %t/12b.elf

; 13. A local-linkage RTTI-disabled vtable is rejected.
; RUN: not ld.lld --image-base=0x100 --section-start=.data=0x100 --section-start=.avm.rtti=0x600 --section-start=.text=0x2000 --section-start=.rodata=0x10000 %t/start.o %t/helper.o %t/local_nortti.o -o %t/13.elf 2>&1 | FileCheck %s --check-prefix=LOCAL-NORTTI

; 13b. A local-linkage RTTI-enabled vtable names its descriptor through a section symbol,
; and it is accepted.
; RUN: llvm-readobj --relocations %t/local_rtti.o | FileCheck %s --check-prefix=LOCAL-SECTREL
; RUN: ld.lld --image-base=0x100 --section-start=.data=0x100 --section-start=.avm.rtti=0x600 --section-start=.text=0x2000 --section-start=.rodata=0x10000 %t/start.o %t/helper.o %t/local_rtti.o -o %t/13b.elf

; 14. Two vtables in one input section are checked over their own ranges, in both orders.
; RUN: not ld.lld --image-base=0x100 --section-start=.data=0x100 --section-start=.avm.rtti=0x600 --section-start=.text=0x2000 --section-start=.rodata=0x10000 %t/start.o %t/helper.o %t/shared.o %t/desc.o -o %t/14.elf 2>&1 | FileCheck %s --check-prefix=SHARED
; RUN: not ld.lld --image-base=0x100 --section-start=.data=0x100 --section-start=.avm.rtti=0x600 --section-start=.text=0x2000 --section-start=.rodata=0x10000 %t/start.o %t/helper.o %t/shared_rev.o %t/desc.o -o %t/14r.elf 2>&1 | FileCheck %s --check-prefix=SHARED

; The helper is dead under GC, so the linker has no dynamic typeid to protect.
; RUN: ld.lld --gc-sections --image-base=0x100 --section-start=.data=0x100 --section-start=.avm.rtti=0x600 --section-start=.text=0x2000 --section-start=.rodata=0x10000 %t/start_nohelper.o %t/helper.o %t/nortti.o --undefined=_ZTV1A -o %t/dead-helper.elf

; NORTTI-V: error: AVM polymorphic RTTI required, but selected vtable '_ZTV1A' has no RTTI entry
; NORTTI-V-NEXT: >>> defined in {{.*}}nortti.o

; WINDOW-V:      Name: _ZTV1A
; WINDOW-V-NEXT: Value: 0x600

; NORTTI-C: error: AVM polymorphic RTTI required, but selected vtable '_ZTC1B0_1A' has no RTTI entry
; NORTTI-C-NEXT: >>> defined in {{.*}}nortti_c.o

; COMDAT-LOSER: error: AVM polymorphic RTTI required, but selected vtable '_ZTV1A' has no RTTI entry
; COMDAT-LOSER-NEXT: >>> defined in {{.*}}nortti_comdat.o

; INVALID-TARGET: error: AVM polymorphic RTTI required, but selected vtable '_ZTV1A' has an invalid RTTI target

; LOCAL-NORTTI: error: AVM polymorphic RTTI required, but selected vtable '_ZTVN12_GLOBAL__N_14BaseE' has no RTTI entry
; LOCAL-NORTTI-NEXT: >>> defined in {{.*}}local_nortti.o

; LOCAL-SECTREL:      R_AVM_PROG24 .avm.rtti.type._ZTIN12_GLOBAL__N_14BaseE 0x0

; SHARED: error: AVM polymorphic RTTI required, but selected vtable '_ZTV1B' has no RTTI entry
; SHARED-NEXT: >>> defined in {{.*}}shared{{(_rev)?}}.o
; SHARED-NOT: _ZTV1A

;--- start.ll
target datalayout = "e-m:e-p:16:8-p1:24:8-i8:8-i16:8-i32:8-i64:8-f16:8-f32:8-n8:16-S8-P1-G0-A0"
target triple = "avm-unknown-arduboyfx"

declare ptr @__avm_typeid_from_object(ptr)

define void @_start() {
  %p = call ptr @__avm_typeid_from_object(ptr null)
  ret void
}

;--- start_nohelper.ll
target datalayout = "e-m:e-p:16:8-p1:24:8-i8:8-i16:8-i32:8-i64:8-f16:8-f32:8-n8:16-S8-P1-G0-A0"
target triple = "avm-unknown-arduboyfx"

define void @_start() {
  ret void
}

;--- start_marker.ll
target datalayout = "e-m:e-p:16:8-p1:24:8-i8:8-i16:8-i32:8-i64:8-f16:8-f32:8-n8:16-S8-P1-G0-A0"
target triple = "avm-unknown-arduboyfx"

; A static typeid marks its descriptor with this weak marker. It has no dynamic helper.
module asm ".weak __avm_rtti_alias._ZTI1A"

define void @_start() {
  ret void
}

;--- helper.ll
target datalayout = "e-m:e-p:16:8-p1:24:8-i8:8-i16:8-i32:8-i64:8-f16:8-f32:8-n8:16-S8-P1-G0-A0"
target triple = "avm-unknown-arduboyfx"

; A stand-in for the runtime helper. Its own section can be kept or collected.
define ptr @__avm_typeid_from_object(ptr %object) {
  ret ptr %object
}

;--- nortti.ll
target datalayout = "e-m:e-p:16:8-p1:24:8-i8:8-i16:8-i32:8-i64:8-f16:8-f32:8-n8:16-S8-P1-G0-A0"
target triple = "avm-unknown-arduboyfx"

; The -fno-rtti vtable. Its RTTI slot is zero, and it has no relocation.
@_ZTV1A = dso_local unnamed_addr addrspace(1) constant [3 x ptr addrspace(1)] [ptr addrspace(1) null, ptr addrspace(1) null, ptr addrspace(1) null], align 1

;--- nortti_dead.ll
target datalayout = "e-m:e-p:16:8-p1:24:8-i8:8-i16:8-i32:8-i64:8-f16:8-f32:8-n8:16-S8-P1-G0-A0"
target triple = "avm-unknown-arduboyfx"

@_ZTV1C = dso_local unnamed_addr addrspace(1) constant [3 x ptr addrspace(1)] [ptr addrspace(1) null, ptr addrspace(1) null, ptr addrspace(1) null], align 1

;--- desc.ll
target datalayout = "e-m:e-p:16:8-p1:24:8-i8:8-i16:8-i32:8-i64:8-f16:8-f32:8-n8:16-S8-P1-G0-A0"
target triple = "avm-unknown-arduboyfx"

; The canonical descriptor. Its six bytes lie in the flash window.
@_ZTI1A = dso_local addrspace(1) constant [6 x i8] zeroinitializer, section ".avm.rtti.type._ZTI1A", align 1

;--- rtti_window.ll
target datalayout = "e-m:e-p:16:8-p1:24:8-i8:8-i16:8-i32:8-i64:8-f16:8-f32:8-n8:16-S8-P1-G0-A0"
target triple = "avm-unknown-arduboyfx"

; The -frtti vtable lies in .avm.rtti, so both the vtable and its descriptor are in the window.
@_ZTV1A = dso_local unnamed_addr addrspace(1) constant [3 x ptr addrspace(1)] [ptr addrspace(1) null, ptr addrspace(1) @_ZTI1A, ptr addrspace(1) null], section ".avm.rtti.vtable._ZTV1A", align 1
@_ZTI1A = external addrspace(1) constant [6 x i8]

;--- rtti.ll
target datalayout = "e-m:e-p:16:8-p1:24:8-i8:8-i16:8-i32:8-i64:8-f16:8-f32:8-n8:16-S8-P1-G0-A0"
target triple = "avm-unknown-arduboyfx"

; The -frtti vtable in .rodata, which is high. Its descriptor is in desc.ll.
@_ZTI1A = external addrspace(1) constant [6 x i8]
@_ZTV1A = dso_local unnamed_addr addrspace(1) constant [3 x ptr addrspace(1)] [ptr addrspace(1) null, ptr addrspace(1) @_ZTI1A, ptr addrspace(1) null], align 1

;--- nortti_c.ll
target datalayout = "e-m:e-p:16:8-p1:24:8-i8:8-i16:8-i32:8-i64:8-f16:8-f32:8-n8:16-S8-P1-G0-A0"
target triple = "avm-unknown-arduboyfx"

@_ZTC1B0_1A = dso_local unnamed_addr addrspace(1) constant [3 x ptr addrspace(1)] [ptr addrspace(1) null, ptr addrspace(1) null, ptr addrspace(1) null], align 1

;--- rtti_c.ll
target datalayout = "e-m:e-p:16:8-p1:24:8-i8:8-i16:8-i32:8-i64:8-f16:8-f32:8-n8:16-S8-P1-G0-A0"
target triple = "avm-unknown-arduboyfx"

@_ZTI1A = external addrspace(1) constant [6 x i8]
@_ZTC1B0_1A = dso_local unnamed_addr addrspace(1) constant [3 x ptr addrspace(1)] [ptr addrspace(1) null, ptr addrspace(1) @_ZTI1A, ptr addrspace(1) null], align 1

;--- rtti_comdat.ll
target datalayout = "e-m:e-p:16:8-p1:24:8-i8:8-i16:8-i32:8-i64:8-f16:8-f32:8-n8:16-S8-P1-G0-A0"
target triple = "avm-unknown-arduboyfx"

$_ZTI1A = comdat any
$_ZTV1A = comdat any
@_ZTI1A = linkonce_odr addrspace(1) constant [6 x i8] zeroinitializer, section ".avm.rtti.type._ZTI1A", comdat, align 1
@_ZTV1A = linkonce_odr unnamed_addr addrspace(1) constant [3 x ptr addrspace(1)] [ptr addrspace(1) null, ptr addrspace(1) @_ZTI1A, ptr addrspace(1) null], comdat, align 1

;--- nortti_comdat.ll
target datalayout = "e-m:e-p:16:8-p1:24:8-i8:8-i16:8-i32:8-i64:8-f16:8-f32:8-n8:16-S8-P1-G0-A0"
target triple = "avm-unknown-arduboyfx"

$_ZTV1A = comdat any
@_ZTV1A = linkonce_odr unnamed_addr addrspace(1) constant [3 x ptr addrspace(1)] [ptr addrspace(1) null, ptr addrspace(1) null, ptr addrspace(1) null], comdat, align 1

;--- weak_rtti.ll
target datalayout = "e-m:e-p:16:8-p1:24:8-i8:8-i16:8-i32:8-i64:8-f16:8-f32:8-n8:16-S8-P1-G0-A0"
target triple = "avm-unknown-arduboyfx"

@_ZTI1A = extern_weak addrspace(1) constant [6 x i8]
@_ZTV1A = dso_local unnamed_addr addrspace(1) constant [3 x ptr addrspace(1)] [ptr addrspace(1) null, ptr addrspace(1) @_ZTI1A, ptr addrspace(1) null], align 1

;--- zero_rtti.ll
target datalayout = "e-m:e-p:16:8-p1:24:8-i8:8-i16:8-i32:8-i64:8-f16:8-f32:8-n8:16-S8-P1-G0-A0"
target triple = "avm-unknown-arduboyfx"

; A descriptor with no bytes has no extent to check, so the vtable's reference to it is invalid.
@_ZTI1A = dso_local addrspace(1) constant [0 x i8] zeroinitializer, section ".avm.rtti.type._ZTI1A", align 1
@_ZTV1A = dso_local unnamed_addr addrspace(1) constant [3 x ptr addrspace(1)] [ptr addrspace(1) null, ptr addrspace(1) @_ZTI1A, ptr addrspace(1) null], align 1

;--- placeholder.ll
target datalayout = "e-m:e-p:16:8-p1:24:8-i8:8-i16:8-i32:8-i64:8-f16:8-f32:8-n8:16-S8-P1-G0-A0"
target triple = "avm-unknown-arduboyfx"

; The runtime's zero-filled kind placeholder, which classifies a descriptor and is not a vtable.
@_ZTVN10__cxxabiv117__class_type_infoE = dso_local addrspace(1) constant [9 x i8] zeroinitializer, align 1

;--- type_info.ll
target datalayout = "e-m:e-p:16:8-p1:24:8-i8:8-i16:8-i32:8-i64:8-f16:8-f32:8-n8:16-S8-P1-G0-A0"
target triple = "avm-unknown-arduboyfx"

; The runtime's own std::type_info vtable, which is compiled without RTTI.
@_ZTVSt9type_info = dso_local unnamed_addr addrspace(1) constant [3 x ptr addrspace(1)] [ptr addrspace(1) null, ptr addrspace(1) null, ptr addrspace(1) null], align 1

;--- local_nortti.ll
target datalayout = "e-m:e-p:16:8-p1:24:8-i8:8-i16:8-i32:8-i64:8-f16:8-f32:8-n8:16-S8-P1-G0-A0"
target triple = "avm-unknown-arduboyfx"

@_ZTVN12_GLOBAL__N_14BaseE = internal unnamed_addr addrspace(1) constant [3 x ptr addrspace(1)] [ptr addrspace(1) null, ptr addrspace(1) null, ptr addrspace(1) null], align 1

;--- local_rtti.ll
target datalayout = "e-m:e-p:16:8-p1:24:8-i8:8-i16:8-i32:8-i64:8-f16:8-f32:8-n8:16-S8-P1-G0-A0"
target triple = "avm-unknown-arduboyfx"

@_ZTIN12_GLOBAL__N_14BaseE = internal addrspace(1) constant [6 x i8] zeroinitializer, section ".avm.rtti.type._ZTIN12_GLOBAL__N_14BaseE", align 1
@_ZTVN12_GLOBAL__N_14BaseE = internal unnamed_addr addrspace(1) constant [3 x ptr addrspace(1)] [ptr addrspace(1) null, ptr addrspace(1) @_ZTIN12_GLOBAL__N_14BaseE, ptr addrspace(1) null], align 1

;--- shared.ll
target datalayout = "e-m:e-p:16:8-p1:24:8-i8:8-i16:8-i32:8-i64:8-f16:8-f32:8-n8:16-S8-P1-G0-A0"
target triple = "avm-unknown-arduboyfx"

; Both vtables share .rodata.pair, and the RTTI-disabled vtable follows the valid one.
@_ZTI1A = external addrspace(1) constant [6 x i8]
@_ZTV1A = dso_local unnamed_addr addrspace(1) constant [3 x ptr addrspace(1)] [ptr addrspace(1) null, ptr addrspace(1) @_ZTI1A, ptr addrspace(1) null], section ".rodata.pair", align 1
@_ZTV1B = dso_local unnamed_addr addrspace(1) constant [3 x ptr addrspace(1)] [ptr addrspace(1) null, ptr addrspace(1) null, ptr addrspace(1) null], section ".rodata.pair", align 1

;--- shared_rev.ll
target datalayout = "e-m:e-p:16:8-p1:24:8-i8:8-i16:8-i32:8-i64:8-f16:8-f32:8-n8:16-S8-P1-G0-A0"
target triple = "avm-unknown-arduboyfx"

; The same pair, with the RTTI-disabled vtable first.
@_ZTI1A = external addrspace(1) constant [6 x i8]
@_ZTV1B = dso_local unnamed_addr addrspace(1) constant [3 x ptr addrspace(1)] [ptr addrspace(1) null, ptr addrspace(1) null, ptr addrspace(1) null], section ".rodata.pair", align 1
@_ZTV1A = dso_local unnamed_addr addrspace(1) constant [3 x ptr addrspace(1)] [ptr addrspace(1) null, ptr addrspace(1) @_ZTI1A, ptr addrspace(1) null], section ".rodata.pair", align 1
