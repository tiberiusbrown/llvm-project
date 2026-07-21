; RUN: llc -mtriple=avm -O2 -verify-machineinstrs < %s | FileCheck %s
; RUN: llc -mtriple=avm -O2 -verify-machineinstrs -filetype=obj %s -o %t.o

define i32 @shl32_16(i32 %value) {
; CHECK-LABEL: shl32_16:
; CHECK-NOT:   call
; CHECK:       mov r5, r4
; CHECK-NEXT:  xor r4, r4
  %result = shl i32 %value, 16
  ret i32 %result
}

define i32 @lshr32_16(i32 %value) {
; CHECK-LABEL: lshr32_16:
; CHECK-NOT:   call
; CHECK:       mov r4, r5
; CHECK-NEXT:  xor r5, r5
  %result = lshr i32 %value, 16
  ret i32 %result
}

define i32 @ashr32_16(i32 %value) {
; CHECK-LABEL: ashr32_16:
; CHECK-NOT:   call
; CHECK:       mov r4, r5
; CHECK-NEXT:  asr16i r5, 15
  %result = ashr i32 %value, 16
  ret i32 %result
}

define i32 @lshr32_variable(i32 %value, i32 %count) {
; CHECK-LABEL: lshr32_variable:
; CHECK:       call{{8|16|f?}} __avm_lshrsi3
  %result = lshr i32 %value, %count
  ret i32 %result
}
