; RUN: llc -mtriple=avm-unknown-arduboyfx -O2 \
; RUN:   -verify-machineinstrs < %s | FileCheck %s

@unsigned_low_result = external global i16
@unsigned_high_result = external global i16
@signed_low_result = external global i16
@signed_high_result = external global i16

define i16 @unsigned_high(i16 %left, i16 %right) {
entry:
; CHECK-LABEL: unsigned_high:
; CHECK: __avm_mulsi3
  %left.wide = zext i16 %left to i32
  %right.wide = zext i16 %right to i32
  %product = mul i32 %left.wide, %right.wide
  %shifted = lshr i32 %product, 16
  %high = trunc i32 %shifted to i16
  ret i16 %high
}

define i16 @signed_high(i16 %left, i16 %right) {
entry:
; CHECK-LABEL: signed_high:
; CHECK: __avm_mulsi3
  %left.wide = sext i16 %left to i32
  %right.wide = sext i16 %right to i32
  %product = mul i32 %left.wide, %right.wide
  %shifted = ashr i32 %product, 16
  %high = trunc i32 %shifted to i16
  ret i16 %high
}

define void @unsigned_low_high(i16 %left, i16 %right) {
entry:
; CHECK-LABEL: unsigned_low_high:
; CHECK: __avm_mulsi3
  %left.wide = zext i16 %left to i32
  %right.wide = zext i16 %right to i32
  %product = mul i32 %left.wide, %right.wide
  %low = trunc i32 %product to i16
  %shifted = lshr i32 %product, 16
  %high = trunc i32 %shifted to i16
  store volatile i16 %low, ptr @unsigned_low_result
  store volatile i16 %high, ptr @unsigned_high_result
  ret void
}

define void @signed_low_high(i16 %left, i16 %right) {
entry:
; CHECK-LABEL: signed_low_high:
; CHECK: __avm_mulsi3
  %left.wide = sext i16 %left to i32
  %right.wide = sext i16 %right to i32
  %product = mul i32 %left.wide, %right.wide
  %low = trunc i32 %product to i16
  %shifted = ashr i32 %product, 16
  %high = trunc i32 %shifted to i16
  store volatile i16 %low, ptr @signed_low_result
  store volatile i16 %high, ptr @signed_high_result
  ret void
}

; CHECK-NOT: mulhu
; CHECK-NOT: mulhs
; CHECK-NOT: umul_lohi
; CHECK-NOT: smul_lohi
