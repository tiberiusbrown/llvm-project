; RUN: llc -mtriple=avm-unknown-arduboyfx -O2 -verify-machineinstrs %s -o %t.s
; RUN: FileCheck %s < %t.s
; RUN: llc -mtriple=avm-unknown-arduboyfx -O2 -verify-machineinstrs -filetype=obj %s -o %t.o

target triple = "avm-unknown-arduboyfx"

define i32 @sext_i8_to_i32(i32 %value) {
; CHECK-LABEL: sext_i8_to_i32:
; CHECK: sext8
entry:
  %byte = trunc i32 %value to i8
  %extended = sext i8 %byte to i32
  ret i32 %extended
}

define i16 @sext_i1_to_i16(i1 zeroext %value) {
; CHECK-LABEL: sext_i1_to_i16:
; CHECK-NOT: ldi8
; CHECK-NOT: and
; CHECK: neg16
; CHECK-NOT: lsl16i
; CHECK-NOT: asr16i
  %extended = sext i1 %value to i16
  ret i16 %extended
}

define i16 @sext_low_bit_i16(i16 %value) {
; CHECK-LABEL: sext_low_bit_i16:
; CHECK: ldi8 [[ONE:r[0-9]+]], 1
; CHECK: and [[REG:r[0-9]+]], [[ONE]]
; CHECK-NEXT: neg16 [[REG]]
; CHECK-NOT: lsl16i
; CHECK-NOT: asr16i
  %bit = trunc i16 %value to i1
  %extended = sext i1 %bit to i16
  ret i16 %extended
}

define i32 @sext_i1_to_i32(i1 zeroext %value) {
; CHECK-LABEL: sext_i1_to_i32:
; CHECK-NOT: ldi8
; CHECK-NOT: and
; CHECK: neg16
  %extended = sext i1 %value to i32
  ret i32 %extended
}

define i32 @sext_low_bit_i32(i32 %value) {
; CHECK-LABEL: sext_low_bit_i32:
; CHECK: ldi8 {{r[0-9]+}}, 1
; CHECK: and [[REG32:r[0-9]+]], {{r[0-9]+}}
; CHECK-NEXT: neg16 [[REG32]]
; CHECK-NOT: lsl16i
  %bit = trunc i32 %value to i1
  %extended = sext i1 %bit to i32
  ret i32 %extended
}
