; RUN: llc -mtriple=avm -O0 -verify-machineinstrs < %s | FileCheck %s
; RUN: llc -mtriple=avm -O2 -verify-machineinstrs < %s | FileCheck %s
; RUN: llc -mtriple=avm -O2 -filetype=obj < %s -o %t
; RUN: llvm-readobj --hex-dump=.avm.stackcalls %t | FileCheck %s --check-prefix=STACK

; Fixed q2 is the normal float argument/return register: no copies or spills.
define float @polynomial(float %x) {
; CHECK-LABEL: polynomial:
; CHECK-NOT: stsp
; CHECK-NOT: call
; CHECK: fpoly 3, 2.0, -3.0, 4.0
; CHECK-NOT: stsp
; CHECK-NOT: call
; CHECK: ret
  %y = call float asm "fpoly 3, 2.0, -3.0, 4.0", "={q2},0"(float %x)
  ret float %y
}
; Stack metadata remains complete (flags=8) for typed inline coefficients.
; STACK: 00000000 00000000 00000000 08
