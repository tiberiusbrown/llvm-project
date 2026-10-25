; RUN: llc -mtriple=avm -O0 -verify-machineinstrs < %s | FileCheck %s
; RUN: llc -mtriple=avm -O2 -verify-machineinstrs < %s | FileCheck %s

; The helpers take two native words and return the complete 32-bit product.
define i32 @unsigned_full(i16 %a, i16 %b) {
; CHECK-LABEL: unsigned_full:
; CHECK-NOT: mov r6,
; CHECK-NOT: mov r7,
; CHECK: {{(call|jmp)}} __avm_umulhisi3
  %x = zext i16 %a to i32
  %y = zext i16 %b to i32
  %product = mul i32 %x, %y
  ret i32 %product
}

define i32 @signed_full(i16 %a, i16 %b) {
; CHECK-LABEL: signed_full:
; CHECK-NOT: asr16i
; CHECK: {{(call|jmp)}} __avm_mulhisi3
  %x = sext i16 %a to i32
  %y = sext i16 %b to i32
  %product = mul i32 %x, %y
  ret i32 %product
}

; Range proofs also apply to masks and extending loads.
define i32 @masked_full(i32 %a, i32 %b) {
; CHECK-LABEL: masked_full:
; CHECK: {{(call|jmp)}} __avm_umulhisi3
  %x = and i32 %a, 65535
  %y = and i32 %b, 65535
  %product = mul i32 %x, %y
  ret i32 %product
}

define i32 @loaded_signed_full(ptr %a, ptr addrspace(1) %b) {
; CHECK-LABEL: loaded_signed_full:
; CHECK: {{(call|jmp)}} __avm_mulhisi3
  %left = load i16, ptr %a, align 1
  %right = load i16, ptr addrspace(1) %b, align 1
  %x = sext i16 %left to i32
  %y = sext i16 %right to i32
  %product = mul i32 %x, %y
  ret i32 %product
}

; A nonnegative signed-word range can share the signed helper.
define i32 @mixed_small_positive(i16 %a, i16 %b) {
; CHECK-LABEL: mixed_small_positive:
; CHECK: {{(call|jmp)}} __avm_mulhisi3
  %positive = and i16 %b, 32767
  %x = sext i16 %a to i32
  %y = zext i16 %positive to i32
  %product = mul i32 %x, %y
  ret i32 %product
}

; These values do not both fit the same signed/unsigned word range.
define i32 @mixed_full(i16 %a, i16 %b) {
; CHECK-LABEL: mixed_full:
; CHECK: {{(call|jmp)}} __avm_mulsi3
  %x = sext i16 %a to i32
  %y = zext i16 %b to i32
  %product = mul i32 %x, %y
  ret i32 %product
}

define i32 @seventeen_bit_full(i32 %a, i16 %b) {
; CHECK-LABEL: seventeen_bit_full:
; CHECK: {{(call|jmp)}} __avm_mulsi3
  %x = and i32 %a, 131071
  %y = zext i16 %b to i32
  %product = mul i32 %x, %y
  ret i32 %product
}

define i32 @general_full(i32 %a, i32 %b) {
; CHECK-LABEL: general_full:
; CHECK: {{(call|jmp)}} __avm_mulsi3
  %product = mul i32 %a, %b
  ret i32 %product
}
