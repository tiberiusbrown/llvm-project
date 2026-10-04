; RUN: opt -mtriple=avm -passes=wholeprogramdevirt -verify-each -S %s | FileCheck %s

target datalayout = "e-m:e-p:16:8-p1:24:8-i8:8-i16:8-i32:8-i64:8-f16:8-f32:8-n8:16-S8-P1-G0-A0"
target triple = "avm"

; Adding return-value bytes must preserve program-space vtables and aliases.
; CHECK: private addrspace(1) constant
; CHECK: @vt1 = alias {{.*}}ptr addrspace(1)
; CHECK: @vt2 = alias {{.*}}ptr addrspace(1)
@vt1 = addrspace(1) constant [1 x ptr addrspace(1)] [ptr addrspace(1) @f1], !type !0, !vcall_visibility !1
@vt2 = addrspace(1) constant [1 x ptr addrspace(1)] [ptr addrspace(1) @f2], !type !0, !vcall_visibility !1

; CHECK-LABEL: define i16 @dispatch
; CHECK: load i16, ptr addrspace(1)
; CHECK-NOT: call addrspace(1) i16
define i16 @dispatch(ptr %obj) addrspace(1) {
  %vt = load ptr addrspace(1), ptr %obj
  %testptr = addrspacecast ptr addrspace(1) %vt to ptr
  %ok = call i1 @llvm.type.test(ptr %testptr, metadata !"base")
  call void @llvm.assume(i1 %ok)
  %fn = load ptr addrspace(1), ptr addrspace(1) %vt
  %r = call addrspace(1) i16 %fn(ptr %obj)
  ret i16 %r
}

define i16 @f1(ptr %obj) addrspace(1) memory(none) { ret i16 1 }
define i16 @f2(ptr %obj) addrspace(1) memory(none) { ret i16 2 }
declare i1 @llvm.type.test(ptr, metadata)
declare void @llvm.assume(i1)
!0 = !{i64 0, !"base"}
!1 = !{i64 2}
