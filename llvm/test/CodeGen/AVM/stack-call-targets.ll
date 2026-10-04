; RUN: llc -mtriple=avm -O0 -verify-machineinstrs %s -o - | FileCheck %s
; RUN: llc -mtriple=avm -O2 -verify-machineinstrs %s -o - | FileCheck %s

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
