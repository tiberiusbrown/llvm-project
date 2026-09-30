; RUN: llc -mtriple=avm -O2 -verify-machineinstrs < %s | FileCheck %s

define i16 @mul3(i16 %x) {
; CHECK-LABEL: mul3:
; CHECK-NOT: mul16
; CHECK: add
; CHECK: add
; CHECK-NOT: mul16
; CHECK: ret
  %product = mul i16 %x, 3
  ret i16 %product
}

define i16 @mul5(i16 %x) {
; CHECK-LABEL: mul5:
; CHECK-NOT: mul16
; CHECK: add
; CHECK-NOT: mul16
; CHECK: ret
  %product = mul i16 %x, 5
  ret i16 %product
}

define i16 @mul257(i16 %x) {
; CHECK-LABEL: mul257:
; CHECK-NOT: mul16
; CHECK: lsl16i {{r[4-7]}}, 8
; CHECK: add
; CHECK-NOT: mul16
; CHECK: ret
  %product = mul i16 %x, 257
  ret i16 %product
}

define i16 @mul5_unsigned_byte(i8 %x) {
; CHECK-LABEL: mul5_unsigned_byte:
; CHECK: mulu8.w
; CHECK-NOT: lsl16i
; CHECK: ret
  %wide = zext i8 %x to i16
  %product = mul i16 %wide, 5
  ret i16 %product
}

define i16 @mul7(i16 %x) {
; CHECK-LABEL: mul7:
; CHECK: mul16
  %product = mul i16 %x, 7
  ret i16 %product
}

define i16 @mul31(i16 %x) {
; CHECK-LABEL: mul31:
; CHECK: mul16
  %product = mul i16 %x, 31
  ret i16 %product
}
