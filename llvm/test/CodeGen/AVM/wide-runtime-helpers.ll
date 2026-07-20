; RUN: llc -mtriple=avm -O0 -verify-machineinstrs < %s | FileCheck %s
; RUN: llc -mtriple=avm -O2 -verify-machineinstrs < %s | FileCheck %s

define i64 @mul64(i64 %left, i64 %right) {
; CHECK-LABEL: mul64:
; CHECK:       call __avm_muldi3
  %result = mul i64 %left, %right
  ret i64 %result
}

define i64 @udiv64(i64 %left, i64 %right) {
; CHECK-LABEL: udiv64:
; CHECK:       call __avm_udivdi3
  %result = udiv i64 %left, %right
  ret i64 %result
}

define i64 @sdiv64(i64 %left, i64 %right) {
; CHECK-LABEL: sdiv64:
; CHECK:       call __avm_divdi3
  %result = sdiv i64 %left, %right
  ret i64 %result
}

define i64 @urem64(i64 %left, i64 %right) {
; CHECK-LABEL: urem64:
; CHECK:       call __avm_umoddi3
  %result = urem i64 %left, %right
  ret i64 %result
}

define i64 @srem64(i64 %left, i64 %right) {
; CHECK-LABEL: srem64:
; CHECK:       call __avm_moddi3
  %result = srem i64 %left, %right
  ret i64 %result
}

define i64 @shl64(i64 %value, i16 %amount) {
; CHECK-LABEL: shl64:
; CHECK:       call __avm_ashldi3
  %wide = zext i16 %amount to i64
  %result = shl i64 %value, %wide
  ret i64 %result
}

define i64 @lshr64(i64 %value, i16 %amount) {
; CHECK-LABEL: lshr64:
; CHECK:       call __avm_lshrdi3
  %wide = zext i16 %amount to i64
  %result = lshr i64 %value, %wide
  ret i64 %result
}

define i64 @ashr64(i64 %value, i16 %amount) {
; CHECK-LABEL: ashr64:
; CHECK:       call __avm_ashrdi3
  %wide = zext i16 %amount to i64
  %result = ashr i64 %value, %wide
  ret i64 %result
}
