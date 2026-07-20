; RUN: llc -mtriple=avm -O0 --frame-pointer=none -verify-machineinstrs \
; RUN:   < %s | FileCheck %s --check-prefixes=CHECK,O0
; RUN: llc -mtriple=avm -O2 --frame-pointer=none -verify-machineinstrs \
; RUN:   < %s | FileCheck %s --check-prefixes=CHECK,O2
; RUN: llc -mtriple=avm -O2 -stop-after=finalize-isel < %s -o - \
; RUN:   | FileCheck %s --check-prefix=MIR
; RUN: opt -mtriple=avm -passes='default<O2>' -S < %s \
; RUN:   | FileCheck %s --check-prefix=OPT

declare ptr @llvm.avm.memcpy(ptr, ptr, i16)
declare ptr @llvm.avm.memset(ptr, i16, i16)
declare ptr @llvm.avm.memmove(ptr, ptr, i16)
declare void @llvm.memcpy.p0.p0.i16(ptr, ptr, i16, i1 immarg)
declare void @llvm.memcpy.p0.p1.i16(ptr, ptr addrspace(1), i16, i1 immarg)
declare void @llvm.memset.p0.i16(ptr, i8, i16, i1 immarg)
declare void @llvm.memmove.p0.p0.i16(ptr, ptr, i16, i1 immarg)

define ptr @target_memcpy(ptr %dst, ptr %src, i16 %size) {
; CHECK-LABEL: target_memcpy:
; CHECK:       sys memcpy
; CHECK-NEXT:  ret
  %result = call ptr @llvm.avm.memcpy(ptr %dst, ptr %src, i16 %size)
  ret ptr %result
}

define ptr @target_memset(ptr %dst, i16 %value, i16 %size) {
; CHECK-LABEL: target_memset:
; CHECK:       sys memset
; CHECK-NEXT:  ret
  %result = call ptr @llvm.avm.memset(ptr %dst, i16 %value, i16 %size)
  ret ptr %result
}

define ptr @target_memmove(ptr %dst, ptr %src, i16 %size) {
; CHECK-LABEL: target_memmove:
; CHECK:       sys memmove
; CHECK-NEXT:  ret
  %result = call ptr @llvm.avm.memmove(ptr %dst, ptr %src, i16 %size)
  ret ptr %result
}

; MIR-LABEL: name: target_memcpy
; MIR:       {{%[0-9]+}}:r4only = SYS_MEMCPY_PSEUDO
; MIR-LABEL: name: target_memset
; MIR:       {{%[0-9]+}}:r4only = SYS_MEMSET_PSEUDO
; MIR-LABEL: name: target_memmove
; MIR:       {{%[0-9]+}}:r4only = SYS_MEMMOVE_PSEUDO

define i16 @memmove_inputs_remain_live(ptr %dst, ptr %src, i16 %size) {
; O0-LABEL: memmove_inputs_remain_live:
; O0:       sys memmove
; O0:       add
; O0:       ret
; O2-LABEL: memmove_inputs_remain_live:
; O2-NOT:   stsp
; O2:       sys memmove
; O2-NEXT:  add r5, r6
; O2-NEXT:  mov r4, r5
; O2-NEXT:  ret
  %result = call ptr @llvm.avm.memmove(ptr %dst, ptr %src, i16 %size)
  %source = ptrtoint ptr %src to i16
  %sum = add i16 %source, %size
  ret i16 %sum
}

define i16 @memset_value_remains_live(ptr %dst, i16 %value, i16 %size) {
; O0-LABEL: memset_value_remains_live:
; O0:       sys memset
; O0:       ret
; O2-LABEL: memset_value_remains_live:
; O2-NOT:   stsp
; O2:       sys memset
; O2-NEXT:  mov r4, r5
; O2-NEXT:  ret
  %result = call ptr @llvm.avm.memset(ptr %dst, i16 %value, i16 %size)
  ret i16 %value
}

define void @generic_memcpy(ptr %dst, ptr %src, i16 %size) {
; CHECK-LABEL: generic_memcpy:
; CHECK:       sys memcpy
  call void @llvm.memcpy.p0.p0.i16(ptr %dst, ptr %src, i16 %size, i1 false),
                                  !alias.scope !3, !noalias !6
  ret void
}

define void @generic_memcpy_p(ptr %dst, ptr addrspace(1) %src, i16 %size) {
; CHECK-LABEL: generic_memcpy_p:
; CHECK-NOT:   zext8
; CHECK:       sys memcpy_p
; CHECK-NOT:   zext8
  call void @llvm.memcpy.p0.p1.i16(ptr %dst, ptr addrspace(1) %src, i16 %size,
                                   i1 false)
  ret void
}

define void @inttoptr_memcpy_p(ptr %dst, i32 %source_bits, i16 %size) {
; CHECK-LABEL: inttoptr_memcpy_p:
; CHECK-NOT:   and
; CHECK-NOT:   zext8
; CHECK:       sys memcpy_p
  %src = inttoptr i32 %source_bits to ptr addrspace(1)
  call void @llvm.memcpy.p0.p1.i16(ptr %dst, ptr addrspace(1) %src, i16 %size,
                                   i1 false)
  ret void
}

define void @generic_memset(ptr %dst, i8 %value, i16 %size) {
; CHECK-LABEL: generic_memset:
; CHECK:       sys memset
  call void @llvm.memset.p0.i16(ptr %dst, i8 %value, i16 %size, i1 false)
  ret void
}

define void @generic_memmove(ptr %dst, ptr %src, i16 %size) {
; CHECK-LABEL: generic_memmove:
; CHECK:       sys memmove
  call void @llvm.memmove.p0.p0.i16(ptr %dst, ptr %src, i16 %size, i1 false)
  ret void
}

define void @large_constant_copy(ptr %dst, ptr %src) {
; CHECK-LABEL: large_constant_copy:
; CHECK:       sys memcpy
  call void @llvm.memcpy.p0.p0.i16(ptr %dst, ptr %src, i16 18, i1 false)
  ret void
}

define void @small_constant_copy(ptr %dst, ptr %src) {
; CHECK-LABEL: small_constant_copy:
; CHECK-NOT:   sys memcpy
; CHECK:       ld{{8u|16}}
; CHECK:       st{{8|16}}
  call void @llvm.memcpy.p0.p0.i16(ptr %dst, ptr %src, i16 2, i1 false)
  ret void
}

define void @small_constant_fill(ptr %dst, i8 %value) {
; CHECK-LABEL: small_constant_fill:
; CHECK-NOT:   sys memset
; CHECK:       st8
; CHECK:       st8
  call void @llvm.memset.p0.i16(ptr %dst, i8 %value, i16 2, i1 false)
  ret void
}

define void @small_constant_move(ptr %dst, ptr %src) {
; CHECK-LABEL: small_constant_move:
; CHECK-NOT:   sys memmove
; CHECK:       ld{{8u|16}}
; CHECK:       st{{8|16}}
  call void @llvm.memmove.p0.p0.i16(ptr %dst, ptr %src, i16 2, i1 false)
  ret void
}

define void @small_constant_program_copy(ptr %dst, ptr addrspace(1) %src) {
; CHECK-LABEL: small_constant_program_copy:
; CHECK-NOT:   sys memcpy_p
; CHECK:       ldp{{8u|16}}
; CHECK:       st{{8|16}}
  call void @llvm.memcpy.p0.p1.i16(ptr %dst, ptr addrspace(1) %src, i16 2,
                                   i1 false)
  ret void
}

define void @large_constant_fill(ptr %dst, i8 %value) {
; CHECK-LABEL: large_constant_fill:
; CHECK:       sys memset
  call void @llvm.memset.p0.i16(ptr %dst, i8 %value, i16 18, i1 false)
  ret void
}

define void @large_constant_move(ptr %dst, ptr %src) {
; CHECK-LABEL: large_constant_move:
; CHECK:       sys memmove
  call void @llvm.memmove.p0.p0.i16(ptr %dst, ptr %src, i16 18, i1 false)
  ret void
}

define void @volatile_copy(ptr %dst, ptr %src, i16 %size) {
; CHECK-LABEL: volatile_copy:
; CHECK-NOT:   sys memcpy
; CHECK:       ld8u
; CHECK:       st8
  call void @llvm.memcpy.p0.p0.i16(ptr %dst, ptr %src, i16 %size, i1 true)
  ret void
}

define void @volatile_fill(ptr %dst, i8 %value, i16 %size) {
; CHECK-LABEL: volatile_fill:
; CHECK-NOT:   sys memset
; CHECK:       st8
  call void @llvm.memset.p0.i16(ptr %dst, i8 %value, i16 %size, i1 true)
  ret void
}

define void @volatile_move(ptr %dst, ptr %src, i16 %size) {
; CHECK-LABEL: volatile_move:
; CHECK-NOT:   sys memmove
; CHECK:       ld8u
; CHECK:       st8
  call void @llvm.memmove.p0.p0.i16(ptr %dst, ptr %src, i16 %size, i1 true)
  ret void
}

define i8 @service_alias_order(ptr %dst, ptr %src, i16 %size) {
; CHECK-LABEL: service_alias_order:
; CHECK:       st8
; CHECK:       sys memcpy
; CHECK:       ld8u
  store i8 7, ptr %dst, align 1
  call void @llvm.memcpy.p0.p0.i16(ptr %dst, ptr %src, i16 %size, i1 false)
  %value = load i8, ptr %src, align 1
  ret i8 %value
}

define void @proven_nonoverlap_move(ptr noalias %dst, ptr noalias %src,
                                    i16 %size) {
; OPT-LABEL: define void @proven_nonoverlap_move(
; OPT:       call{{.*}} void @llvm.memcpy.p0.p0.i16(
; OPT-NOT:   llvm.memmove
  call void @llvm.memmove.p0.p0.i16(ptr %dst, ptr %src, i16 %size, i1 false)
  ret void
}

; MIR-LABEL: name: generic_memcpy
; MIR:        SYS_MEMCPY_PSEUDO
; MIR-SAME:   :: (store unknown-size
; MIR-SAME:   !alias.scope !
; MIR-SAME:   !noalias !
; MIR-SAME:   (load unknown-size
; MIR-LABEL: name: generic_memcpy_p
; MIR:        SYS_MEMCPY_P_PSEUDO
; MIR-SAME:   :: (store unknown-size
; MIR-SAME:   (load unknown-size
; MIR-SAME:   addrspace 1
; MIR-LABEL: name: generic_memset
; MIR:        SYS_MEMSET_PSEUDO
; MIR-SAME:   :: (store unknown-size
; MIR-NOT:    (load unknown-size
; MIR-LABEL: name: generic_memmove
; MIR:        SYS_MEMMOVE_PSEUDO
; MIR-SAME:   :: (store unknown-size
; MIR-SAME:   (load unknown-size

!0 = distinct !{!0, !"service domain"}
!3 = !{!4}
!4 = distinct !{!4, !0, !"destination"}
!6 = !{!7}
!7 = distinct !{!7, !0, !"source"}
