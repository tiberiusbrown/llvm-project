; RUN: opt -mtriple=avm -passes=wholeprogramdevirt -S %s | FileCheck %s
; RUN: opt -mtriple=avm -passes=wholeprogramdevirt -devirtualize-speculatively -S %s | FileCheck %s --check-prefix=SPEC

@vt1 = constant [1 x ptr addrspace(1)] [ptr addrspace(1) @f1], !type !0, !vcall_visibility !1
@vt2 = constant [1 x ptr addrspace(1)] [ptr addrspace(1) @f2], !type !0, !vcall_visibility !1
@vt3 = constant [1 x ptr addrspace(1)] [ptr addrspace(1) @f3], !type !0, !vcall_visibility !1
@vt4 = constant [1 x ptr addrspace(1)] [ptr addrspace(1) @f4], !type !0, !vcall_visibility !1
@vt5 = constant [1 x ptr addrspace(1)] [ptr addrspace(1) @f5], !type !0, !vcall_visibility !1

; Five implementations exceed CVP's small-set limit. WPD must preserve all.
; CHECK-LABEL: define void @dispatch
; CHECK: call addrspace(1) void %fn(ptr %obj), !callees ![[SET:[0-9]+]]
; CHECK: ![[SET]] = !{ptr addrspace(1) @f1, ptr addrspace(1) @f2, ptr addrspace(1) @f3, ptr addrspace(1) @f4, ptr addrspace(1) @f5}
; SPEC-LABEL: define void @dispatch
; SPEC-NOT: !callees
define void @dispatch(ptr %obj) addrspace(1) {
  %vt = load ptr, ptr %obj
  %ok = call i1 @llvm.type.test(ptr %vt, metadata !"base")
  call void @llvm.assume(i1 %ok)
  %fn = load ptr addrspace(1), ptr %vt
  call addrspace(1) void %fn(ptr %obj)
  ret void
}

declare void @f1(ptr) addrspace(1)
declare void @f2(ptr) addrspace(1)
declare void @f3(ptr) addrspace(1)
declare void @f4(ptr) addrspace(1)
declare void @f5(ptr) addrspace(1)
declare i1 @llvm.type.test(ptr, metadata)
declare void @llvm.assume(i1)
!0 = !{i64 0, !"base"}
!1 = !{i64 2}
