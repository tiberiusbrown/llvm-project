; RUN: llc -mtriple=avm -O0 -filetype=obj %s -o %t.o
; RUN: llvm-readobj --symbols %t.o | FileCheck %s
; RUN: llc -mtriple=avm -O2 -filetype=obj %s -o %t.opt.o
; RUN: llvm-readobj --symbols %t.opt.o | FileCheck %s
; CHECK: Name: __avm_crt_termination_phase
; CHECK-NEXT: Value: 0x9FE
; CHECK: Section: Absolute
; No caller record is subtracted from the loader's initial SP. The root's
; normal fixed-frame metadata still includes this two-byte object.
define void @_start() addrspace(1) {
  %phase = alloca i16, align 1, !avm.termination.phase !0
  store volatile i16 0, ptr %phase, align 1
  ret void
}
!0 = !{}
