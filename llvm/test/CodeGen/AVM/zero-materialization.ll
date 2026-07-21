; RUN: llc -mtriple=avm -O2 -verify-machineinstrs < %s | FileCheck %s

define i16 @return_zero_i16() {
; CHECK-LABEL: return_zero_i16:
; CHECK:       xor r4, r4
; CHECK-NOT:   ldi{{8|16}} r4, 0
  ret i16 0
}

define i32 @return_zero_i32() {
; CHECK-LABEL: return_zero_i32:
; CHECK:       xor r4, r4
; CHECK-NEXT:  xor r5, r5
; CHECK-NOT:   ldi{{8|16}} r{{4|5}}, 0
  ret i32 0
}

define i32 @zext_i16_to_i32(i16 %value) {
; CHECK-LABEL: zext_i16_to_i32:
; CHECK-NOT:   nop
; CHECK-NOT:   mov r4, r4
; CHECK:       xor r5, r5
; CHECK-NOT:   ldi{{8|16}} r5, 0
  %result = zext i16 %value to i32
  ret i32 %result
}

define i16 @return_zero_i16_optsize() #0 {
; CHECK-LABEL: return_zero_i16_optsize:
; CHECK:       xor r4, r4
; CHECK-NOT:   ldi{{8|16}} r4, 0
  ret i16 0
}

define i32 @return_zero_i32_optsize() #0 {
; CHECK-LABEL: return_zero_i32_optsize:
; CHECK:       xor r4, r4
; CHECK-NEXT:  xor r5, r5
; CHECK-NOT:   ldi{{8|16}} r{{4|5}}, 0
  ret i32 0
}

define i32 @zext_i16_to_i32_optsize(i16 %value) #0 {
; CHECK-LABEL: zext_i16_to_i32_optsize:
; CHECK-NOT:   nop
; CHECK-NOT:   mov r4, r4
; CHECK:       xor r5, r5
; CHECK-NOT:   ldi{{8|16}} r5, 0
  %result = zext i16 %value to i32
  ret i32 %result
}

define i32 @sext_i16_to_i32(i16 %value) {
; CHECK-LABEL: sext_i16_to_i32:
; CHECK-NOT:   nop
; CHECK-NOT:   mov r4, r4
; CHECK:       mov r5, r4
; CHECK-NEXT:  asr16i r5, 15
  %result = sext i16 %value to i32
  ret i32 %result
}

define i32 @sext_i16_to_i32_optsize(i16 %value) #0 {
; CHECK-LABEL: sext_i16_to_i32_optsize:
; CHECK-NOT:   nop
; CHECK-NOT:   mov r4, r4
; CHECK:       mov r5, r4
; CHECK-NEXT:  asr16i r5, 15
  %result = sext i16 %value to i32
  ret i32 %result
}

attributes #0 = { optsize }
