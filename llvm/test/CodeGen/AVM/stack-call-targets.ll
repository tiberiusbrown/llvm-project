; RUN: llc -mtriple=avm -O0 -verify-machineinstrs %s -o - | FileCheck %s
; RUN: llc -mtriple=avm -O2 -verify-machineinstrs %s -o - | FileCheck %s

target datalayout = "e-m:e-p:16:8-p1:24:8-i8:8-i16:8-i32:8-i64:8-f16:8-f32:8-n8:16-S8-P1-G0-A0"

; Proven sets produce alternative direct metadata edges while the executable
; still uses an indirect call. Unknown pointers retain the incomplete edge.
; CHECK-LABEL: annotated:
; CHECK: callp
; CHECK: .section .avm.stackcalls
; CHECK: .3byte small
; CHECK-NEXT: .short 0
; CHECK-NEXT: .byte 0
; CHECK: .3byte large
; CHECK-NEXT: .short 0
; CHECK-NEXT: .byte 0
define void @annotated(ptr addrspace(1) %f) addrspace(1) {
  call addrspace(1) void %f(), !callees !0
  call addrspace(1) void @barrier()
  ret void
}

; CHECK-LABEL: propagated:
; CHECK: jmpp
; CHECK: .section .avm.stackcalls
; CHECK: .3byte large
; CHECK-NEXT: .short 0
; CHECK-NEXT: .byte 2
; CHECK: .3byte small
; CHECK-NEXT: .short 0
; CHECK-NEXT: .byte 2
define void @propagated(i1 %which) addrspace(1) {
  %f = select i1 %which, ptr addrspace(1) @small, ptr addrspace(1) @large
  tail call addrspace(1) void %f()
  ret void
}

; CHECK-LABEL: unknown:
; CHECK: jmpp
; CHECK: .section .avm.stackcalls
; CHECK: .short 1
; CHECK-NEXT: .byte 0
; CHECK-NEXT: .short 0
; CHECK-NEXT: .byte 3
define void @unknown(ptr addrspace(1) %f) addrspace(1) {
  tail call addrspace(1) void %f()
  ret void
}

declare void @small() addrspace(1)
declare void @large() addrspace(1)
declare void @barrier() addrspace(1)
!0 = !{ptr addrspace(1) @small, ptr addrspace(1) @large}

@dispatch = internal addrspace(1) constant [3 x ptr addrspace(1)] [ptr addrspace(1) @small, ptr addrspace(1) null, ptr addrspace(1) @large]

; CHECK-LABEL: constant_table:
; CHECK: jmpp
; CHECK: .section .avm.stackcalls
; CHECK: .3byte large
; CHECK-NEXT: .short 0
; CHECK-NEXT: .byte 2
; CHECK: .3byte small
; CHECK-NEXT: .short 0
; CHECK-NEXT: .byte 2
define void @constant_table(i16 %index) addrspace(1) {
  %entry = getelementptr [3 x ptr addrspace(1)], ptr addrspace(1) @dispatch, i16 0, i16 %index
  %target = load ptr addrspace(1), ptr addrspace(1) %entry, align 1
  tail call addrspace(1) void %target()
  ret void
}

; Unaligned byte addressing must not be mistaken for a whole pointer element.
; CHECK-LABEL: table_bytes:
; CHECK: jmpp
; CHECK: .section .avm.stackcalls
; CHECK: .short 1
; CHECK-NEXT: .byte 0
; CHECK-NEXT: .short 0
; CHECK-NEXT: .byte 3
define void @table_bytes(i16 %index) addrspace(1) {
  %entry = getelementptr i8, ptr addrspace(1) @dispatch, i16 %index
  %target = load ptr addrspace(1), ptr addrspace(1) %entry, align 1
  tail call addrspace(1) void %target()
  ret void
}

; An unrestricted 24-bit index may wrap the stride multiplication.
; CHECK-LABEL: table_wrapped:
; CHECK: jmpp
; CHECK: .section .avm.stackcalls
; CHECK: .short 1
; CHECK-NEXT: .byte 0
; CHECK-NEXT: .short 0
; CHECK-NEXT: .byte 3
define void @table_wrapped(i24 %index) addrspace(1) {
  %entry = getelementptr [3 x i8], ptr addrspace(1) @dispatch, i24 %index
  %target = load ptr addrspace(1), ptr addrspace(1) %entry, align 1
  tail call addrspace(1) void %target()
  ret void
}
