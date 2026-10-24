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
; NARROW: {{(call|jmp)}} __avm_mulsi3
  %wide = sext i8 %index to i32
  %product = mul i32 %wide, 6
  ret i32 %product
}

define i32 @unrestricted_product(i32 %left, i32 %right) {
; NARROW-LABEL: unrestricted_product:
; NARROW: {{(call|jmp)}} __avm_mulsi3
  %product = mul i32 %left, %right
  ret i32 %product
}

