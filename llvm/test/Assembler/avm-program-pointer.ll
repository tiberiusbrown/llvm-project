; RUN: llvm-as < %s | llvm-dis | FileCheck %s

target datalayout = "e-m:e-p:16:8-p1:24:8-i8:8-i16:8-i32:8-i64:8-f16:8-f32:8-n8:16-S8-P1-G0-A0"
target triple = "avm-unknown-arduboyfx"

; CHECK: declare void @llvm.memcpy.p0.p1.i16(ptr noalias writeonly captures(none),
; CHECK-SAME: ptr addrspace(1) noalias readonly captures(none), i16, i1 immarg)
declare void @llvm.memcpy.p0.p1.i16(ptr noalias nocapture writeonly,
                                    ptr addrspace(1) noalias nocapture readonly,
                                    i16, i1 immarg)

; CHECK-LABEL: define void @copy_from_program_memory(
; CHECK: call addrspace(1) void @llvm.memcpy.p0.p1.i16(
define void @copy_from_program_memory(ptr %destination,
                                      ptr addrspace(1) %source,
                                      i16 %size) {
  call void @llvm.memcpy.p0.p1.i16(ptr align 1 %destination,
                                   ptr addrspace(1) align 1 %source,
                                   i16 %size, i1 false)
  ret void
}
