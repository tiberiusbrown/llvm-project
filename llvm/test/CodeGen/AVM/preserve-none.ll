; RUN: llc -mtriple=avm -O0 -verify-machineinstrs %s -o - | FileCheck %s
; RUN: llc -mtriple=avm -O2 -verify-machineinstrs %s -o - | FileCheck %s

define preserve_nonecc void @clobber_scratch() nounwind {
; CHECK-LABEL: clobber_scratch:
; CHECK-NOT: push16
; CHECK: ;APP
; CHECK: ;NO_APP
; CHECK-NOT: pop16
; CHECK: ret
; CHECK: .Lfunc_end
  call void asm sideeffect "", "~{r0},~{r1},~{r2}"()
  ret void
}

define preserve_nonecc void @preserve_frame_pointer() nounwind {
; CHECK-LABEL: preserve_frame_pointer:
; CHECK: push16 r3
; CHECK: ;APP
; CHECK: ;NO_APP
; CHECK: pop16 r3
; CHECK: ret
  call void asm sideeffect "", "~{r3}"()
  ret void
}

declare preserve_nonecc i16 @external(i16)

define i16 @c_wrapper(i16 %x) nounwind {
; CHECK-LABEL: c_wrapper:
; CHECK: push16 r2
; CHECK: push16 r1
; CHECK: push16 r0
; CHECK: call external
; CHECK: pop16 r0
; CHECK: pop16 r1
; CHECK: pop16 r2
; CHECK: ret
; CHECK: .Lfunc_end
  %result = tail call preserve_nonecc i16 @external(i16 %x)
  ret i16 %result
}

define preserve_nonecc ptr addrspace(1) @program_pointer(ptr addrspace(1) %p) nounwind {
; CHECK-LABEL: program_pointer:
; CHECK-NOT: push16
; CHECK: ret
; CHECK: .Lfunc_end
  ret ptr addrspace(1) %p
}
