; RUN: llc -mtriple=avm -O2 -verify-machineinstrs < %s \
; RUN:   | FileCheck %s

define i16 @select_mask_bit(i16 %selector, i16 %true, i16 %false) {
; CHECK-LABEL: select_mask_bit:
; CHECK:       and
; CHECK:       tst8
; CHECK-NOT:   tst16
; CHECK:       cmov.eq
  %masked = and i16 %selector, 1
  %condition = icmp ne i16 %masked, 0
  %result = select i1 %condition, i16 %true, i16 %false
  ret i16 %result
}

define i1 @signed_word_test(i16 %value) {
; CHECK-LABEL: signed_word_test:
; CHECK-NOT:   tst8
; CHECK:       lsr16i
  %condition = icmp slt i16 %value, 0
  ret i1 %condition
}
