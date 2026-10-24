; RUN: llc -mtriple=avm -O2 -verify-machineinstrs < %s | FileCheck %s

; The compare is legalized through a Boolean, then selected against zero.
; Ordinary arithmetic preserves CC, so its original condition is sufficient.
define i16 @clamp_wrapped_coordinate(i16 %value) {
; CHECK-LABEL: clamp_wrapped_coordinate:
; CHECK:       cmp r{{[0-7]}}, r{{[0-7]}}
; CHECK-NOT:   cset
; CHECK-NOT:   tst
; CHECK:       cmov.ult
; CHECK-NOT:   cset
; CHECK-NOT:   tst
; CHECK:       ret
  %offset = add i16 %value, -128
  %wrapped = icmp ult i16 %value, %offset
  %result = select i1 %wrapped, i16 0, i16 %offset
  ret i16 %result
}

define i16 @clamp_wrapped_coordinate_optsize(i16 %value) optsize {
; CHECK-LABEL: clamp_wrapped_coordinate_optsize:
; CHECK:       cmp r{{[0-7]}}, r{{[0-7]}}
; CHECK-NOT:   cset
; CHECK-NOT:   tst
; CHECK:       cmov.ult
; CHECK-NOT:   cset
; CHECK-NOT:   tst
; CHECK:       ret
  %offset = add i16 %value, -64
  %wrapped = icmp ult i16 %value, %offset
  %result = select i1 %wrapped, i16 0, i16 %offset
  ret i16 %result
}
