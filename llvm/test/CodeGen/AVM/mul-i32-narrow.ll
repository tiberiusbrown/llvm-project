; RUN: llc -mtriple=avm -O2 -verify-machineinstrs < %s | FileCheck %s --implicit-check-not="{{(call|jmp)}} __avm_mulsi3" --check-prefix=NARROW

; Both the input and the full product fit in a native word. In particular,
; 255 * 6 = 1530 must not be truncated to a byte or sign-extended.
define i32 @byte_times_six(i8 %index) {
; NARROW-LABEL: byte_times_six:
; NARROW: mulu8.w
; NARROW: {{(xor|sub)}} r5, r5
; NARROW: ret
  %wide = zext i8 %index to i32
  %offset = mul i32 %wide, 6
  ret i32 %offset
}

; Known bits also recover the range from masks rather than explicit extensions.
define i32 @masked_times_six(i32 %index) {
; NARROW-LABEL: masked_times_six:
; NARROW: mulu8.w
; NARROW: ret
  %byte = and i32 %index, 255
  %offset = mul i32 %byte, 6
  ret i32 %offset
}

define i32 @byte_product(i8 %left, i8 %right) {
; NARROW-LABEL: byte_product:
; NARROW: mulu8.w
; NARROW: ret
  %a = zext i8 %left to i32
  %b = zext i8 %right to i32
  %product = mul i32 %a, %b
  ret i32 %product
}

; The bound is inclusive: 255 * 257 = 65535 needs an unsigned word result.
define i32 @byte_times_257(i8 %index) {
; NARROW-LABEL: byte_times_257:
; NARROW: ret
  %wide = zext i8 %index to i32
  %product = mul i32 %wide, 257
  ret i32 %product
}

define i32 @ten_bit_times_six(i32 %index) {
; NARROW-LABEL: ten_bit_times_six:
; NARROW: mul16
; NARROW: ret
  %masked = and i32 %index, 1023
  %product = mul i32 %masked, 6
  ret i32 %product
}

; Preserve the 24-bit program address and use the zero-extended word offset.
%record = type { ptr addrspace(1), ptr addrspace(1) }
@records = external addrspace(1) constant [256 x %record], align 1

define ptr addrspace(1) @record_title(i8 %index) {
; NARROW-LABEL: record_title:
; NARROW: mulu8.w
; NARROW: add32
; NARROW: ldp24
; NARROW: ret
  %wide = zext i8 %index to i32
  %address = getelementptr [256 x %record], ptr addrspace(1) @records, i32 0, i32 %wide, i32 1
  %title = load ptr addrspace(1), ptr addrspace(1) %address, align 1
  ret ptr addrspace(1) %title
}

; These products must retain their full width.
define i32 @byte_times_258(i8 %index) {
; NARROW-LABEL: byte_times_258:
; NARROW: {{(call|jmp)}} __avm_mulsi3
  %wide = zext i8 %index to i32
  %product = mul i32 %wide, 258
  ret i32 %product
}

define i32 @word_times_six(i16 %index) {
; NARROW-LABEL: word_times_six:
; NARROW: {{(call|jmp)}} __avm_mulsi3
  %wide = zext i16 %index to i32
  %product = mul i32 %wide, 6
  ret i32 %product
}

define i32 @signed_byte_times_six(i8 %index) {
; NARROW-LABEL: signed_byte_times_six:
; NARROW: mulsu8.w
; NARROW: asr16i r5, 15
; NARROW: ret
  %wide = sext i8 %index to i32
  %product = mul i32 %wide, 6
  ret i32 %product
}

; The most negative byte times 257 does not fit signed i16. Its high word
; must come from the original operand, not from the overflowing low product.
define i32 @signed_byte_times_257(i8 %index) {
; NARROW-LABEL: signed_byte_times_257:
; NARROW: mov [[SIGN:r[4-7]]], r4
; NARROW: sext8 [[SIGN]]
; NARROW: lsl16i [[LOW:r[4-7]]], 8
; NARROW: add [[LOW]], [[SIGN]]
; NARROW: asr16i [[SIGN]], 15
; NARROW: ret
  %wide = sext i8 %index to i32
  %product = mul i32 %wide, 257
  ret i32 %product
}

; Arithmetic can distribute the multiplication over this bias. Keep the
; unsigned narrow product native in that form as well.
define i32 @biased_byte_times_257(i32 %index) {
; NARROW-LABEL: biased_byte_times_257:
; NARROW: lsl16i {{r[4-7]}}, 8
; NARROW: or
; NARROW: add32
; NARROW: ret
  %byte = and i32 %index, 255
  %signed = add nsw i32 %byte, -128
  %product = mul i32 %signed, 257
  ret i32 %product
}

; Generalize to a different narrow range and to 2^k - 1 constants.
define i32 @signed_seven_bits_times_1023(i7 %index) {
; NARROW-LABEL: signed_seven_bits_times_1023:
; NARROW: lsl16i {{r[4-7]}}, 10
; NARROW: sub
; NARROW: ret
  %wide = sext i7 %index to i32
  %product = mul i32 %wide, 1023
  ret i32 %product
}

; Signed variable products whose complete result fits a word use widening
; byte multiplication, then sign-extend the result.
define i32 @signed_byte_product(i8 %left, i8 %right) {
; NARROW-LABEL: signed_byte_product:
; NARROW: muls8.w
; NARROW: asr16i r5, 15
; NARROW: ret
  %a = sext i8 %left to i32
  %b = sext i8 %right to i32
  %product = mul i32 %a, %b
  ret i32 %product
}

; A nonzero positive range, rather than just a constant, preserves the sign.
define i32 @signed_byte_times_positive_word(i8 %left, i16 %right) {
; NARROW-LABEL: signed_byte_times_positive_word:
; NARROW: mul16
; NARROW: asr16i {{r[4-7]}}, 15
; NARROW: ret
  %a = sext i8 %left to i32
  %masked = and i16 %right, 255
  %positive = or i16 %masked, 256
  %b = zext i16 %positive to i32
  %product = mul i32 %a, %b
  ret i32 %product
}

; Guard the 17-bit bound, sign changes, and multiplication by zero.
define i32 @signed_byte_times_513(i8 %index) {
; NARROW-LABEL: signed_byte_times_513:
; NARROW: {{(call|jmp)}} __avm_mulsi3
  %wide = sext i8 %index to i32
  %product = mul i32 %wide, 513
  ret i32 %product
}

define i32 @signed_byte_times_negative_257(i8 %index) {
; NARROW-LABEL: signed_byte_times_negative_257:
; NARROW: {{(call|jmp)}} __avm_mulsi3
  %wide = sext i8 %index to i32
  %product = mul i32 %wide, -257
  ret i32 %product
}

define i32 @signed_byte_times_zero_through_511(i8 %left, i16 %right) {
; NARROW-LABEL: signed_byte_times_zero_through_511:
; NARROW: {{(call|jmp)}} __avm_mulsi3
  %a = sext i8 %left to i32
  %masked = and i16 %right, 511
  %b = zext i16 %masked to i32
  %product = mul i32 %a, %b
  ret i32 %product
}

define i32 @signed_word_times_257(i16 %index) {
; NARROW-LABEL: signed_word_times_257:
; NARROW: {{(call|jmp)}} __avm_mulsi3
  %wide = sext i16 %index to i32
  %product = mul i32 %wide, 257
  ret i32 %product
}

define i32 @unrestricted_product(i32 %left, i32 %right) {
; NARROW-LABEL: unrestricted_product:
; NARROW: {{(call|jmp)}} __avm_mulsi3
  %product = mul i32 %left, %right
  ret i32 %product
}

