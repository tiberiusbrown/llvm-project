; RUN: llc -mtriple=avm -O0 -verify-machineinstrs %s -o - | FileCheck %s
; RUN: llc -mtriple=avm -O2 -verify-machineinstrs %s -o - | FileCheck %s

; Every defined constant i32 shift count must stay in native code, including
; zero, word transfers at 16, and the last defined count at 31. Runtime
; variable-shift helper improvements must not turn these into helper calls.

define i32 @shl_0(i32 %value) {
; CHECK-LABEL: shl_0:
; CHECK-NOT: __avm_
; CHECK: ret
  %result = shl i32 %value, 0
  ret i32 %result
}

define i32 @shl_1(i32 %value) {
; CHECK-LABEL: shl_1:
; CHECK-NOT: __avm_
; CHECK: ret
  %result = shl i32 %value, 1
  ret i32 %result
}

define i32 @shl_2(i32 %value) {
; CHECK-LABEL: shl_2:
; CHECK-NOT: __avm_
; CHECK: ret
  %result = shl i32 %value, 2
  ret i32 %result
}

define i32 @shl_3(i32 %value) {
; CHECK-LABEL: shl_3:
; CHECK-NOT: __avm_
; CHECK: ret
  %result = shl i32 %value, 3
  ret i32 %result
}

define i32 @shl_4(i32 %value) {
; CHECK-LABEL: shl_4:
; CHECK-NOT: __avm_
; CHECK: ret
  %result = shl i32 %value, 4
  ret i32 %result
}

define i32 @shl_5(i32 %value) {
; CHECK-LABEL: shl_5:
; CHECK-NOT: __avm_
; CHECK: ret
  %result = shl i32 %value, 5
  ret i32 %result
}

define i32 @shl_6(i32 %value) {
; CHECK-LABEL: shl_6:
; CHECK-NOT: __avm_
; CHECK: ret
  %result = shl i32 %value, 6
  ret i32 %result
}

define i32 @shl_7(i32 %value) {
; CHECK-LABEL: shl_7:
; CHECK-NOT: __avm_
; CHECK: ret
  %result = shl i32 %value, 7
  ret i32 %result
}

define i32 @shl_8(i32 %value) {
; CHECK-LABEL: shl_8:
; CHECK-NOT: __avm_
; CHECK: ret
  %result = shl i32 %value, 8
  ret i32 %result
}

define i32 @shl_9(i32 %value) {
; CHECK-LABEL: shl_9:
; CHECK-NOT: __avm_
; CHECK: ret
  %result = shl i32 %value, 9
  ret i32 %result
}

define i32 @shl_10(i32 %value) {
; CHECK-LABEL: shl_10:
; CHECK-NOT: __avm_
; CHECK: ret
  %result = shl i32 %value, 10
  ret i32 %result
}

define i32 @shl_11(i32 %value) {
; CHECK-LABEL: shl_11:
; CHECK-NOT: __avm_
; CHECK: ret
  %result = shl i32 %value, 11
  ret i32 %result
}

define i32 @shl_12(i32 %value) {
; CHECK-LABEL: shl_12:
; CHECK-NOT: __avm_
; CHECK: ret
  %result = shl i32 %value, 12
  ret i32 %result
}

define i32 @shl_13(i32 %value) {
; CHECK-LABEL: shl_13:
; CHECK-NOT: __avm_
; CHECK: ret
  %result = shl i32 %value, 13
  ret i32 %result
}

define i32 @shl_14(i32 %value) {
; CHECK-LABEL: shl_14:
; CHECK-NOT: __avm_
; CHECK: ret
  %result = shl i32 %value, 14
  ret i32 %result
}

define i32 @shl_15(i32 %value) {
; CHECK-LABEL: shl_15:
; CHECK-NOT: __avm_
; CHECK: ret
  %result = shl i32 %value, 15
  ret i32 %result
}

define i32 @shl_16(i32 %value) {
; CHECK-LABEL: shl_16:
; CHECK-NOT: __avm_
; CHECK: ret
  %result = shl i32 %value, 16
  ret i32 %result
}

define i32 @shl_17(i32 %value) {
; CHECK-LABEL: shl_17:
; CHECK-NOT: __avm_
; CHECK: ret
  %result = shl i32 %value, 17
  ret i32 %result
}

define i32 @shl_18(i32 %value) {
; CHECK-LABEL: shl_18:
; CHECK-NOT: __avm_
; CHECK: ret
  %result = shl i32 %value, 18
  ret i32 %result
}

define i32 @shl_19(i32 %value) {
; CHECK-LABEL: shl_19:
; CHECK-NOT: __avm_
; CHECK: ret
  %result = shl i32 %value, 19
  ret i32 %result
}

define i32 @shl_20(i32 %value) {
; CHECK-LABEL: shl_20:
; CHECK-NOT: __avm_
; CHECK: ret
  %result = shl i32 %value, 20
  ret i32 %result
}

define i32 @shl_21(i32 %value) {
; CHECK-LABEL: shl_21:
; CHECK-NOT: __avm_
; CHECK: ret
  %result = shl i32 %value, 21
  ret i32 %result
}

define i32 @shl_22(i32 %value) {
; CHECK-LABEL: shl_22:
; CHECK-NOT: __avm_
; CHECK: ret
  %result = shl i32 %value, 22
  ret i32 %result
}

define i32 @shl_23(i32 %value) {
; CHECK-LABEL: shl_23:
; CHECK-NOT: __avm_
; CHECK: ret
  %result = shl i32 %value, 23
  ret i32 %result
}

define i32 @shl_24(i32 %value) {
; CHECK-LABEL: shl_24:
; CHECK-NOT: __avm_
; CHECK: ret
  %result = shl i32 %value, 24
  ret i32 %result
}

define i32 @shl_25(i32 %value) {
; CHECK-LABEL: shl_25:
; CHECK-NOT: __avm_
; CHECK: ret
  %result = shl i32 %value, 25
  ret i32 %result
}

define i32 @shl_26(i32 %value) {
; CHECK-LABEL: shl_26:
; CHECK-NOT: __avm_
; CHECK: ret
  %result = shl i32 %value, 26
  ret i32 %result
}

define i32 @shl_27(i32 %value) {
; CHECK-LABEL: shl_27:
; CHECK-NOT: __avm_
; CHECK: ret
  %result = shl i32 %value, 27
  ret i32 %result
}

define i32 @shl_28(i32 %value) {
; CHECK-LABEL: shl_28:
; CHECK-NOT: __avm_
; CHECK: ret
  %result = shl i32 %value, 28
  ret i32 %result
}

define i32 @shl_29(i32 %value) {
; CHECK-LABEL: shl_29:
; CHECK-NOT: __avm_
; CHECK: ret
  %result = shl i32 %value, 29
  ret i32 %result
}

define i32 @shl_30(i32 %value) {
; CHECK-LABEL: shl_30:
; CHECK-NOT: __avm_
; CHECK: ret
  %result = shl i32 %value, 30
  ret i32 %result
}

define i32 @shl_31(i32 %value) {
; CHECK-LABEL: shl_31:
; CHECK-NOT: __avm_
; CHECK: ret
  %result = shl i32 %value, 31
  ret i32 %result
}

define i32 @lshr_0(i32 %value) {
; CHECK-LABEL: lshr_0:
; CHECK-NOT: __avm_
; CHECK: ret
  %result = lshr i32 %value, 0
  ret i32 %result
}

define i32 @lshr_1(i32 %value) {
; CHECK-LABEL: lshr_1:
; CHECK-NOT: __avm_
; CHECK: ret
  %result = lshr i32 %value, 1
  ret i32 %result
}

define i32 @lshr_2(i32 %value) {
; CHECK-LABEL: lshr_2:
; CHECK-NOT: __avm_
; CHECK: ret
  %result = lshr i32 %value, 2
  ret i32 %result
}

define i32 @lshr_3(i32 %value) {
; CHECK-LABEL: lshr_3:
; CHECK-NOT: __avm_
; CHECK: ret
  %result = lshr i32 %value, 3
  ret i32 %result
}

define i32 @lshr_4(i32 %value) {
; CHECK-LABEL: lshr_4:
; CHECK-NOT: __avm_
; CHECK: ret
  %result = lshr i32 %value, 4
  ret i32 %result
}

define i32 @lshr_5(i32 %value) {
; CHECK-LABEL: lshr_5:
; CHECK-NOT: __avm_
; CHECK: ret
  %result = lshr i32 %value, 5
  ret i32 %result
}

define i32 @lshr_6(i32 %value) {
; CHECK-LABEL: lshr_6:
; CHECK-NOT: __avm_
; CHECK: ret
  %result = lshr i32 %value, 6
  ret i32 %result
}

define i32 @lshr_7(i32 %value) {
; CHECK-LABEL: lshr_7:
; CHECK-NOT: __avm_
; CHECK: ret
  %result = lshr i32 %value, 7
  ret i32 %result
}

define i32 @lshr_8(i32 %value) {
; CHECK-LABEL: lshr_8:
; CHECK-NOT: __avm_
; CHECK: ret
  %result = lshr i32 %value, 8
  ret i32 %result
}

define i32 @lshr_9(i32 %value) {
; CHECK-LABEL: lshr_9:
; CHECK-NOT: __avm_
; CHECK: ret
  %result = lshr i32 %value, 9
  ret i32 %result
}

define i32 @lshr_10(i32 %value) {
; CHECK-LABEL: lshr_10:
; CHECK-NOT: __avm_
; CHECK: ret
  %result = lshr i32 %value, 10
  ret i32 %result
}

define i32 @lshr_11(i32 %value) {
; CHECK-LABEL: lshr_11:
; CHECK-NOT: __avm_
; CHECK: ret
  %result = lshr i32 %value, 11
  ret i32 %result
}

define i32 @lshr_12(i32 %value) {
; CHECK-LABEL: lshr_12:
; CHECK-NOT: __avm_
; CHECK: ret
  %result = lshr i32 %value, 12
  ret i32 %result
}

define i32 @lshr_13(i32 %value) {
; CHECK-LABEL: lshr_13:
; CHECK-NOT: __avm_
; CHECK: ret
  %result = lshr i32 %value, 13
  ret i32 %result
}

define i32 @lshr_14(i32 %value) {
; CHECK-LABEL: lshr_14:
; CHECK-NOT: __avm_
; CHECK: ret
  %result = lshr i32 %value, 14
  ret i32 %result
}

define i32 @lshr_15(i32 %value) {
; CHECK-LABEL: lshr_15:
; CHECK-NOT: __avm_
; CHECK: ret
  %result = lshr i32 %value, 15
  ret i32 %result
}

define i32 @lshr_16(i32 %value) {
; CHECK-LABEL: lshr_16:
; CHECK-NOT: __avm_
; CHECK: ret
  %result = lshr i32 %value, 16
  ret i32 %result
}

define i32 @lshr_17(i32 %value) {
; CHECK-LABEL: lshr_17:
; CHECK-NOT: __avm_
; CHECK: ret
  %result = lshr i32 %value, 17
  ret i32 %result
}

define i32 @lshr_18(i32 %value) {
; CHECK-LABEL: lshr_18:
; CHECK-NOT: __avm_
; CHECK: ret
  %result = lshr i32 %value, 18
  ret i32 %result
}

define i32 @lshr_19(i32 %value) {
; CHECK-LABEL: lshr_19:
; CHECK-NOT: __avm_
; CHECK: ret
  %result = lshr i32 %value, 19
  ret i32 %result
}

define i32 @lshr_20(i32 %value) {
; CHECK-LABEL: lshr_20:
; CHECK-NOT: __avm_
; CHECK: ret
  %result = lshr i32 %value, 20
  ret i32 %result
}

define i32 @lshr_21(i32 %value) {
; CHECK-LABEL: lshr_21:
; CHECK-NOT: __avm_
; CHECK: ret
  %result = lshr i32 %value, 21
  ret i32 %result
}

define i32 @lshr_22(i32 %value) {
; CHECK-LABEL: lshr_22:
; CHECK-NOT: __avm_
; CHECK: ret
  %result = lshr i32 %value, 22
  ret i32 %result
}

define i32 @lshr_23(i32 %value) {
; CHECK-LABEL: lshr_23:
; CHECK-NOT: __avm_
; CHECK: ret
  %result = lshr i32 %value, 23
  ret i32 %result
}

define i32 @lshr_24(i32 %value) {
; CHECK-LABEL: lshr_24:
; CHECK-NOT: __avm_
; CHECK: ret
  %result = lshr i32 %value, 24
  ret i32 %result
}

define i32 @lshr_25(i32 %value) {
; CHECK-LABEL: lshr_25:
; CHECK-NOT: __avm_
; CHECK: ret
  %result = lshr i32 %value, 25
  ret i32 %result
}

define i32 @lshr_26(i32 %value) {
; CHECK-LABEL: lshr_26:
; CHECK-NOT: __avm_
; CHECK: ret
  %result = lshr i32 %value, 26
  ret i32 %result
}

define i32 @lshr_27(i32 %value) {
; CHECK-LABEL: lshr_27:
; CHECK-NOT: __avm_
; CHECK: ret
  %result = lshr i32 %value, 27
  ret i32 %result
}

define i32 @lshr_28(i32 %value) {
; CHECK-LABEL: lshr_28:
; CHECK-NOT: __avm_
; CHECK: ret
  %result = lshr i32 %value, 28
  ret i32 %result
}

define i32 @lshr_29(i32 %value) {
; CHECK-LABEL: lshr_29:
; CHECK-NOT: __avm_
; CHECK: ret
  %result = lshr i32 %value, 29
  ret i32 %result
}

define i32 @lshr_30(i32 %value) {
; CHECK-LABEL: lshr_30:
; CHECK-NOT: __avm_
; CHECK: ret
  %result = lshr i32 %value, 30
  ret i32 %result
}

define i32 @lshr_31(i32 %value) {
; CHECK-LABEL: lshr_31:
; CHECK-NOT: __avm_
; CHECK: ret
  %result = lshr i32 %value, 31
  ret i32 %result
}

define i32 @ashr_0(i32 %value) {
; CHECK-LABEL: ashr_0:
; CHECK-NOT: __avm_
; CHECK: ret
  %result = ashr i32 %value, 0
  ret i32 %result
}

define i32 @ashr_1(i32 %value) {
; CHECK-LABEL: ashr_1:
; CHECK-NOT: __avm_
; CHECK: ret
  %result = ashr i32 %value, 1
  ret i32 %result
}

define i32 @ashr_2(i32 %value) {
; CHECK-LABEL: ashr_2:
; CHECK-NOT: __avm_
; CHECK: ret
  %result = ashr i32 %value, 2
  ret i32 %result
}

define i32 @ashr_3(i32 %value) {
; CHECK-LABEL: ashr_3:
; CHECK-NOT: __avm_
; CHECK: ret
  %result = ashr i32 %value, 3
  ret i32 %result
}

define i32 @ashr_4(i32 %value) {
; CHECK-LABEL: ashr_4:
; CHECK-NOT: __avm_
; CHECK: ret
  %result = ashr i32 %value, 4
  ret i32 %result
}

define i32 @ashr_5(i32 %value) {
; CHECK-LABEL: ashr_5:
; CHECK-NOT: __avm_
; CHECK: ret
  %result = ashr i32 %value, 5
  ret i32 %result
}

define i32 @ashr_6(i32 %value) {
; CHECK-LABEL: ashr_6:
; CHECK-NOT: __avm_
; CHECK: ret
  %result = ashr i32 %value, 6
  ret i32 %result
}

define i32 @ashr_7(i32 %value) {
; CHECK-LABEL: ashr_7:
; CHECK-NOT: __avm_
; CHECK: ret
  %result = ashr i32 %value, 7
  ret i32 %result
}

define i32 @ashr_8(i32 %value) {
; CHECK-LABEL: ashr_8:
; CHECK-NOT: __avm_
; CHECK: ret
  %result = ashr i32 %value, 8
  ret i32 %result
}

define i32 @ashr_9(i32 %value) {
; CHECK-LABEL: ashr_9:
; CHECK-NOT: __avm_
; CHECK: ret
  %result = ashr i32 %value, 9
  ret i32 %result
}

define i32 @ashr_10(i32 %value) {
; CHECK-LABEL: ashr_10:
; CHECK-NOT: __avm_
; CHECK: ret
  %result = ashr i32 %value, 10
  ret i32 %result
}

define i32 @ashr_11(i32 %value) {
; CHECK-LABEL: ashr_11:
; CHECK-NOT: __avm_
; CHECK: ret
  %result = ashr i32 %value, 11
  ret i32 %result
}

define i32 @ashr_12(i32 %value) {
; CHECK-LABEL: ashr_12:
; CHECK-NOT: __avm_
; CHECK: ret
  %result = ashr i32 %value, 12
  ret i32 %result
}

define i32 @ashr_13(i32 %value) {
; CHECK-LABEL: ashr_13:
; CHECK-NOT: __avm_
; CHECK: ret
  %result = ashr i32 %value, 13
  ret i32 %result
}

define i32 @ashr_14(i32 %value) {
; CHECK-LABEL: ashr_14:
; CHECK-NOT: __avm_
; CHECK: ret
  %result = ashr i32 %value, 14
  ret i32 %result
}

define i32 @ashr_15(i32 %value) {
; CHECK-LABEL: ashr_15:
; CHECK-NOT: __avm_
; CHECK: ret
  %result = ashr i32 %value, 15
  ret i32 %result
}

define i32 @ashr_16(i32 %value) {
; CHECK-LABEL: ashr_16:
; CHECK-NOT: __avm_
; CHECK: ret
  %result = ashr i32 %value, 16
  ret i32 %result
}

define i32 @ashr_17(i32 %value) {
; CHECK-LABEL: ashr_17:
; CHECK-NOT: __avm_
; CHECK: ret
  %result = ashr i32 %value, 17
  ret i32 %result
}

define i32 @ashr_18(i32 %value) {
; CHECK-LABEL: ashr_18:
; CHECK-NOT: __avm_
; CHECK: ret
  %result = ashr i32 %value, 18
  ret i32 %result
}

define i32 @ashr_19(i32 %value) {
; CHECK-LABEL: ashr_19:
; CHECK-NOT: __avm_
; CHECK: ret
  %result = ashr i32 %value, 19
  ret i32 %result
}

define i32 @ashr_20(i32 %value) {
; CHECK-LABEL: ashr_20:
; CHECK-NOT: __avm_
; CHECK: ret
  %result = ashr i32 %value, 20
  ret i32 %result
}

define i32 @ashr_21(i32 %value) {
; CHECK-LABEL: ashr_21:
; CHECK-NOT: __avm_
; CHECK: ret
  %result = ashr i32 %value, 21
  ret i32 %result
}

define i32 @ashr_22(i32 %value) {
; CHECK-LABEL: ashr_22:
; CHECK-NOT: __avm_
; CHECK: ret
  %result = ashr i32 %value, 22
  ret i32 %result
}

define i32 @ashr_23(i32 %value) {
; CHECK-LABEL: ashr_23:
; CHECK-NOT: __avm_
; CHECK: ret
  %result = ashr i32 %value, 23
  ret i32 %result
}

define i32 @ashr_24(i32 %value) {
; CHECK-LABEL: ashr_24:
; CHECK-NOT: __avm_
; CHECK: ret
  %result = ashr i32 %value, 24
  ret i32 %result
}

define i32 @ashr_25(i32 %value) {
; CHECK-LABEL: ashr_25:
; CHECK-NOT: __avm_
; CHECK: ret
  %result = ashr i32 %value, 25
  ret i32 %result
}

define i32 @ashr_26(i32 %value) {
; CHECK-LABEL: ashr_26:
; CHECK-NOT: __avm_
; CHECK: ret
  %result = ashr i32 %value, 26
  ret i32 %result
}

define i32 @ashr_27(i32 %value) {
; CHECK-LABEL: ashr_27:
; CHECK-NOT: __avm_
; CHECK: ret
  %result = ashr i32 %value, 27
  ret i32 %result
}

define i32 @ashr_28(i32 %value) {
; CHECK-LABEL: ashr_28:
; CHECK-NOT: __avm_
; CHECK: ret
  %result = ashr i32 %value, 28
  ret i32 %result
}

define i32 @ashr_29(i32 %value) {
; CHECK-LABEL: ashr_29:
; CHECK-NOT: __avm_
; CHECK: ret
  %result = ashr i32 %value, 29
  ret i32 %result
}

define i32 @ashr_30(i32 %value) {
; CHECK-LABEL: ashr_30:
; CHECK-NOT: __avm_
; CHECK: ret
  %result = ashr i32 %value, 30
  ret i32 %result
}

define i32 @ashr_31(i32 %value) {
; CHECK-LABEL: ashr_31:
; CHECK-NOT: __avm_
; CHECK: ret
  %result = ashr i32 %value, 31
  ret i32 %result
}
