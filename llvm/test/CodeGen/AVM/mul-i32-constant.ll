; RUN: llc -mtriple=avm -O0 -verify-machineinstrs %s -o - | FileCheck %s
; RUN: llc -mtriple=avm -O2 -verify-machineinstrs %s -o - | FileCheck %s
; RUN: llc -mtriple=avm -O2 -verify-machineinstrs %s -o - | FileCheck %s --check-prefix=NARROW

; Full-width constant products must be lowered before choosing a runtime
; helper. Small shifts use measured ADD32 costs instead of word shifts and
; carry merges. Check the entire body for both helper calls and tail calls.
define i32 @times_three(i32 %x) {
; CHECK-LABEL: times_three:
; CHECK-NOT: {{(call|jmp)}}
; CHECK: add32 [[SCALED:q[0-3]]], [[SCALED]]
; CHECK-NEXT: add32
; CHECK-NEXT: ret
  %r = mul i32 %x, 3
  ret i32 %r
}

define i32 @times_five(i32 %x) {
; CHECK-LABEL: times_five:
; CHECK-NOT: {{(call|jmp)}}
; CHECK: add32 [[SCALED:q[0-3]]], [[SCALED]]
; CHECK-NEXT: add32 [[SCALED]], [[SCALED]]
; CHECK-NEXT: add32
; CHECK-NEXT: ret
  %r = mul i32 %x, 5
  ret i32 %r
}

define i32 @times_nine(i32 %x) {
; CHECK-LABEL: times_nine:
; CHECK-NOT: {{(call|jmp)}}
; CHECK: add32 [[SCALED:q[0-3]]], [[SCALED]]
; CHECK-NEXT: add32 [[SCALED]], [[SCALED]]
; CHECK-NEXT: add32 [[SCALED]], [[SCALED]]
; CHECK-NEXT: add32
; CHECK-NEXT: ret
  %r = mul i32 %x, 9
  ret i32 %r
}

define i32 @times_fifteen(i32 %x) {
; CHECK-LABEL: times_fifteen:
; CHECK-NOT: {{(call|jmp)}}
; CHECK: add32 [[SCALED:q[0-3]]], [[SCALED]]
; CHECK-NEXT: add32 [[SCALED]], [[SCALED]]
; CHECK-NEXT: add32 [[SCALED]], [[SCALED]]
; CHECK-NEXT: add32 [[SCALED]], [[SCALED]]
; CHECK-NEXT: sub32
; CHECK-NOT: {{(call|jmp)}}
; CHECK: ret
  %r = mul i32 %x, 15
  ret i32 %r
}

define i32 @times_seventeen(i32 %x) {
; CHECK-LABEL: times_seventeen:
; CHECK-NOT: {{(call|jmp)}}
; CHECK: add32 [[SCALED:q[0-3]]], [[SCALED]]
; CHECK-NEXT: add32 [[SCALED]], [[SCALED]]
; CHECK-NEXT: add32 [[SCALED]], [[SCALED]]
; CHECK-NEXT: add32 [[SCALED]], [[SCALED]]
; CHECK-NEXT: add32
; CHECK-NEXT: ret
  %r = mul i32 %x, 17
  ret i32 %r
}

; Regression for clamp_signed((int32_t)b * 3 + a): the signed word's
; product exceeds both signed and unsigned word ranges.
define i32 @signed_word_times_three(i16 %x, i16 %a) {
; CHECK-LABEL: signed_word_times_three:
; CHECK-NOT: {{(call|jmp)}}
; CHECK: asr16i
; CHECK: add32
; CHECK-NEXT: add32
; CHECK-NOT: {{(call|jmp)}}
; CHECK: add32
; CHECK-NOT: {{(call|jmp)}}
; CHECK: ret
  %wide = sext i16 %x to i32
  %product = mul nsw i32 %wide, 3
  %addend = zext i16 %a to i32
  %r = add nsw i32 %product, %addend
  ret i32 %r
}

; A word transfer is cheaper than sixteen doublings.
define i32 @times_65537(i32 %x) {
; CHECK-LABEL: times_65537:
; CHECK-NOT: {{(call|jmp)}}
; CHECK: add32
; CHECK-NEXT: ret
  %r = mul i32 65537, %x
  ret i32 %r
}

define i32 @times_negative_three(i32 %x) {
; CHECK-LABEL: times_negative_three:
; CHECK-NOT: {{(call|jmp)}}
; CHECK: add32
; CHECK-NEXT: add32
; CHECK-NOT: {{(call|jmp)}}
; CHECK: sub32
; CHECK-NEXT: ret
  %r = mul i32 %x, -3
  ret i32 %r
}

define i32 @times_negative_fifteen(i32 %x) {
; CHECK-LABEL: times_negative_fifteen:
; CHECK-NOT: {{(call|jmp)}}
; CHECK: add32 [[SCALED:q[0-3]]], [[SCALED]]
; CHECK-NEXT: add32 [[SCALED]], [[SCALED]]
; CHECK-NEXT: add32 [[SCALED]], [[SCALED]]
; CHECK-NEXT: add32 [[SCALED]], [[SCALED]]
; CHECK-NEXT: sub32
; CHECK-NEXT: ret
  %r = mul i32 %x, -15
  ret i32 %r
}

; Pure powers of two stay native, including the sign-bit constant.
define i32 @times_four(i32 %x) {
; CHECK-LABEL: times_four:
; CHECK-NOT: {{(call|jmp)}}
; CHECK: ret
  %r = mul i32 %x, 4
  ret i32 %r
}

define i32 @times_sign_bit(i32 %x) {
; CHECK-LABEL: times_sign_bit:
; CHECK-NOT: {{(call|jmp)}}
; CHECK: ret
  %r = mul i32 %x, -2147483648
  ret i32 %r
}

; Do not expand expensive shifts or arbitrary constants unconditionally.
define i32 @times_31(i32 %x) {
; CHECK-LABEL: times_31:
; CHECK: {{(call|jmp)}} __avm_mulsi3
  %r = mul i32 %x, 31
  ret i32 %r
}

define i32 @times_dense_constant(i32 %x) {
; CHECK-LABEL: times_dense_constant:
; CHECK: {{(call|jmp)}} __avm_mulsi3
  %r = mul i32 %x, 1431655765
  ret i32 %r
}

; Retain the native widening-byte multiply when the entire product fits.
define i32 @byte_times_five(i8 %x) {
; NARROW-LABEL: byte_times_five:
; NARROW-NOT: add32
; NARROW: mulu8.w
; NARROW-NOT: add32
; NARROW: ret
  %wide = zext i8 %x to i32
  %r = mul i32 %wide, 5
  ret i32 %r
}
