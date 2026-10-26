; RUN: llc -mtriple=avm -O2 -verify-machineinstrs < %s | FileCheck %s
; RUN: llc -mtriple=avm -O2 -verify-machineinstrs -filetype=obj %s -o %t.o

; Only the differing word needs a conditional move. This applies to either
; half of an integer or float, without changing the floating representation.
define i32 @integer_common_low(i16 %a, i16 %b) {
; CHECK-LABEL: integer_common_low:
; CHECK-NOT:   adjsp
; CHECK:       cmov.ult
; CHECK-NOT:   cmov
; CHECK:       ret
  %c = icmp ult i16 %a, %b
  %r = select i1 %c, i32 305419896, i32 -1698933128
  ret i32 %r
}

define i32 @integer_common_high(i16 %a, i16 %b) {
; CHECK-LABEL: integer_common_high:
; CHECK-NOT:   adjsp
; CHECK:       cmov.slt
; CHECK-NOT:   cmov
; CHECK:       ret
  %c = icmp slt i16 %a, %b
  %r = select i1 %c, i32 305419896, i32 305441741
  ret i32 %r
}

define float @float_common_low(i32 %a) {
; CHECK-LABEL: float_common_low:
; CHECK-NOT:   adjsp
; CHECK:       cmov.slt
; CHECK-NOT:   cmov
; CHECK:       ret
  %c = icmp slt i32 %a, 0
  %r = select i1 %c, float 0x3FF42468A0000000, float 0xBFF42468A0000000
  ret float %r
}

define float @float_common_high(float %a, float %b) {
; CHECK-LABEL: float_common_high:
; CHECK-NOT:   adjsp
; CHECK:       fcmp
; CHECK:       cmov.eq
; CHECK-NOT:   cmov
; CHECK:       ret
  %c = fcmp olt float %a, %b
  %r = select i1 %c, float 0x3FF0024680000000, float 0x3FF01579A0000000
  ret float %r
}

define float @signed_zero(i16 %a, i16 %b) {
; CHECK-LABEL: signed_zero:
; CHECK-NOT:   adjsp
; CHECK:       cmov.ult
; CHECK-NOT:   cmov
; CHECK:       ret
  %c = icmp ult i16 %a, %b
  %r = select i1 %c, float 0.0, float -0.0
  ret float %r
}

define float @nan_payload(i16 %a, i16 %b) {
; CHECK-LABEL: nan_payload:
; CHECK-NOT:   adjsp
; CHECK:       cmov.ult
; CHECK-NOT:   cmov
; CHECK:       ret
  %c = icmp ult i16 %a, %b
  %r = select i1 %c, float 0x7FF82468A0000000, float 0xFFF82468A0000000
  ret float %r
}

; No word in common: keep the existing full-width selection.
define i32 @integer_different_words(i16 %a, i16 %b) {
; CHECK-LABEL: integer_different_words:
; CHECK:       cmov.ult
; CHECK:       cmov.ult
; CHECK-NOT:   cmov
; CHECK:       ret
  %c = icmp ult i16 %a, %b
  %r = select i1 %c, i32 305419896, i32 -1698898193
  ret i32 %r
}
