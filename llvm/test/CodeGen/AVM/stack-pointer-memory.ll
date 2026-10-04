; RUN: llc -mtriple=avm -O0 -verify-machineinstrs %s -o - | FileCheck %s
; RUN: llc -mtriple=avm -O2 -verify-machineinstrs %s -o - | FileCheck %s
; RUN: llc -mtriple=avm -O0 -avm-stack-target-limit=4 %s -o - | FileCheck %s --check-prefix=LIMIT
; RUN: llc -mtriple=avm -O0 -avm-stack-analysis-iterations=0 %s -o - | FileCheck %s --check-prefix=ITERATIONS

target datalayout = "e-P1-p:16:8-p1:24:8-i8:8-i16:8-i24:8-i32:8-i64:8-f32:8-f64:8-a:8-n8:16-S8"
%object = type { i16, ptr addrspace(1) }

define void @construct_and_read(i1 %which) addrspace(1) {
  %obj = alloca %object, align 1
  %f = select i1 %which, ptr addrspace(1) @small, ptr addrspace(1) @large
  call addrspace(1) void @construct(ptr %obj, ptr addrspace(1) %f)
  call addrspace(1) void @read_field(ptr %obj)
  ret void
}
define internal void @construct(ptr %obj, ptr addrspace(1) %f) addrspace(1) {
  %field = getelementptr %object, ptr %obj, i16 0, i32 1
  store ptr addrspace(1) %f, ptr %field, align 1
  ; An unrelated scalar field must not taint the function-pointer field.
  store i16 42, ptr %obj, align 1
  ret void
}
; CHECK-LABEL: read_field:
; CHECK: .3byte large
; CHECK: .3byte small
; ITERATIONS-LABEL: read_field:
; ITERATIONS: .short 3
; ITERATIONS-NEXT: .byte 0
; ITERATIONS-NEXT: .short 0
; ITERATIONS-NEXT: .byte {{[13]}}
define internal void @read_field(ptr %obj) addrspace(1) {
  %field = getelementptr %object, ptr %obj, i16 0, i32 1
  %f = load ptr addrspace(1), ptr %field, align 1
  call addrspace(1) void %f()
  ret void
}

define void @copy_and_read() addrspace(1) {
  %src = alloca %object, align 1
  %dst = alloca %object, align 1
  %field = getelementptr %object, ptr %src, i16 0, i32 1
  store ptr addrspace(1) @large, ptr %field, align 1
  call void @llvm.memcpy.p0.p0.i16(ptr %dst, ptr %src, i16 5, i1 false)
  call addrspace(1) void @read_copy(ptr %dst)
  ret void
}
; CHECK-LABEL: read_copy:
; CHECK: .3byte large
define internal void @read_copy(ptr %obj) addrspace(1) {
  %field = getelementptr %object, ptr %obj, i16 0, i32 1
  %f = load ptr addrspace(1), ptr %field, align 1
  call addrspace(1) void %f()
  ret void
}

define void @escape_and_read() addrspace(1) {
  %obj = alloca %object, align 1
  %field = getelementptr %object, ptr %obj, i16 0, i32 1
  store ptr addrspace(1) @small, ptr %field, align 1
  call addrspace(1) void @unknown_writer(ptr %obj)
  call addrspace(1) void @read_escape(ptr %obj)
  ret void
}
; CHECK-LABEL: read_escape:
; CHECK: .short 1
; CHECK-NEXT: .byte 0
; CHECK-NEXT: .short 0
; CHECK-NEXT: .byte {{[13]}}
define internal void @read_escape(ptr %obj) addrspace(1) {
  %field = getelementptr %object, ptr %obj, i16 0, i32 1
  %f = load ptr addrspace(1), ptr %field, align 1
  call addrspace(1) void %f()
  ret void
}

define void @five_targets(i1 %a, i1 %b, i1 %c, i1 %d) addrspace(1) {
  %slot = alloca ptr addrspace(1), align 1
  %f1 = select i1 %a, ptr addrspace(1) @small, ptr addrspace(1) @large
  %f2 = select i1 %b, ptr addrspace(1) @third, ptr addrspace(1) %f1
  %f3 = select i1 %c, ptr addrspace(1) @fourth, ptr addrspace(1) %f2
  %f4 = select i1 %d, ptr addrspace(1) @fifth, ptr addrspace(1) %f3
  store ptr addrspace(1) %f4, ptr %slot, align 1
  %loaded = load ptr addrspace(1), ptr %slot, align 1
  call addrspace(1) void %loaded()
  ret void
}
; CHECK-LABEL: five_targets:
; CHECK: .3byte fifth
; CHECK: .3byte fourth
; CHECK: .3byte large
; CHECK: .3byte small
; CHECK: .3byte third
; LIMIT-LABEL: five_targets:
; LIMIT: .short 2
; LIMIT-NEXT: .byte 0
; LIMIT-NEXT: .short 0
; LIMIT-NEXT: .byte {{[13]}}

declare void @small() addrspace(1)
declare void @large() addrspace(1)
declare void @third() addrspace(1)
declare void @fourth() addrspace(1)
declare void @fifth() addrspace(1)
declare void @unknown_writer(ptr) addrspace(1)
declare void @llvm.memcpy.p0.p0.i16(ptr, ptr, i16, i1)
