; RUN: llc -mtriple=abc < %s | FileCheck %s

@fmt = addrspace(1) constant [4 x i8] c"%u\0A\00"

declare void @debug_printf(ptr addrspace(1), ...)

define void @caller() {
entry:
  call void (ptr addrspace(1), ...) @debug_printf(
      ptr addrspace(1) getelementptr inbounds ([4 x i8], ptr addrspace(1) @fmt, i16 0, i16 0),
      i16 42)
  ret void
}

; CHECK-LABEL: caller:
; CHECK: push 42
; CHECK: pushl fmt
; CHECK: call debug_printf
