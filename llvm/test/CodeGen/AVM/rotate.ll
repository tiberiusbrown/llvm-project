; RUN: llc -mtriple=avm-unknown-arduboyfx -O0 -verify-machineinstrs %s -o - | FileCheck %s
; RUN: llc -mtriple=avm-unknown-arduboyfx -O2 -verify-machineinstrs %s -o - | FileCheck %s
; RUN: llc -mtriple=avm-unknown-arduboyfx -O2 -verify-machineinstrs -filetype=obj %s -o %t.o

target triple = "avm-unknown-arduboyfx"

declare i16 @llvm.fshl.i16(i16, i16, i16)
declare i16 @llvm.fshr.i16(i16, i16, i16)
declare i32 @llvm.fshl.i32(i32, i32, i32)
declare i32 @llvm.fshr.i32(i32, i32, i32)

define i16 @rotl16(i16 %value, i16 %count) {
; CHECK-LABEL: rotl16:
; CHECK-NOT: __avm_
; CHECK: ret
entry:
  %result = call i16 @llvm.fshl.i16(
      i16 %value, i16 %value, i16 %count)
  ret i16 %result
}

define i16 @rotr16(i16 %value, i16 %count) {
; CHECK-LABEL: rotr16:
; CHECK-NOT: __avm_
; CHECK: ret
entry:
  %result = call i16 @llvm.fshr.i16(
      i16 %value, i16 %value, i16 %count)
  ret i16 %result
}

define i32 @rotl32(i32 %value, i32 %count) {
; CHECK-LABEL: rotl32:
; CHECK-NOT: __avm_ash
; CHECK-NOT: __avm_lshr
; CHECK: {{(call|jmp)}}{{.*}} __avm_rotlsi3
; CHECK-NOT: {{(call|jmp)}}{{.*}} __avm_
entry:
  %result = call i32 @llvm.fshl.i32(
      i32 %value, i32 %value, i32 %count)
  ret i32 %result
}

define i32 @rotr32(i32 %value, i32 %count) {
; CHECK-LABEL: rotr32:
; CHECK-NOT: __avm_ash
; CHECK-NOT: __avm_lshr
; CHECK: {{(call|jmp)}}{{.*}} __avm_rotrsi3
; CHECK-NOT: {{(call|jmp)}}{{.*}} __avm_
entry:
  %result = call i32 @llvm.fshr.i32(
      i32 %value, i32 %value, i32 %count)
  ret i32 %result
}

; Defined shift/OR idioms must reach the same lowering as the intrinsics.

define i32 @rotate_left_idiom(i32 %value, i32 %count) {
; CHECK-LABEL: rotate_left_idiom:
; CHECK-NOT: __avm_ash
; CHECK-NOT: __avm_lshr
; CHECK: {{(call|jmp)}}{{.*}} __avm_rotlsi3
; CHECK-NOT: {{(call|jmp)}}{{.*}} __avm_
  %other = sub i32 32, %count
  %a = shl i32 %value, %count
  %b = lshr i32 %value, %other
  %result = or i32 %a, %b
  ret i32 %result
}

define i32 @rotate_right_idiom(i32 %value, i32 %count) {
; CHECK-LABEL: rotate_right_idiom:
; CHECK-NOT: __avm_ash
; CHECK-NOT: __avm_lshr
; CHECK: {{(call|jmp)}}{{.*}} __avm_rotrsi3
; CHECK-NOT: {{(call|jmp)}}{{.*}} __avm_
  %other = sub i32 32, %count
  %a = lshr i32 %value, %count
  %b = shl i32 %value, %other
  %result = or i32 %a, %b
  ret i32 %result
}

; Different integer promotions must not hide the complementary counts.
define i32 @rotl32_byte_count(i32 %value, i8 %count) {
; CHECK-LABEL: rotl32_byte_count:
; CHECK-NOT: __avm_ash
; CHECK-NOT: __avm_lshr
; CHECK: {{(call|jmp)}}{{.*}} __avm_rotlsi3
; CHECK-NOT: {{(call|jmp)}}{{.*}} __avm_
  %wide = zext i8 %count to i32
  %word = zext i8 %count to i16
  %other = sub i16 32, %word
  %other_wide = zext i16 %other to i32
  %a = shl i32 %value, %wide
  %b = lshr i32 %value, %other_wide
  %result = or i32 %a, %b
  ret i32 %result
}

define i32 @rotr32_byte_count(i32 %value, i8 %count) {
; CHECK-LABEL: rotr32_byte_count:
; CHECK-NOT: __avm_ash
; CHECK-NOT: __avm_lshr
; CHECK: {{(call|jmp)}}{{.*}} __avm_rotrsi3
; CHECK-NOT: {{(call|jmp)}}{{.*}} __avm_
  %wide = zext i8 %count to i32
  %word = zext i8 %count to i16
  %other = sub i16 32, %word
  %other_wide = zext i16 %other to i32
  %a = lshr i32 %value, %wide
  %b = shl i32 %value, %other_wide
  %result = or i32 %a, %b
  ret i32 %result
}

; Narrowing below five bits changes the count and must not form a rotate.
define i32 @not_rotate_narrow_count(i32 %value, i8 %count) {
; CHECK-LABEL: not_rotate_narrow_count:
; CHECK-NOT: __avm_rot
; CHECK: ret
  %wide = zext i8 %count to i32
  %narrow = trunc i8 %count to i4
  %word = zext i4 %narrow to i16
  %other = sub i16 32, %word
  %other_wide = zext i16 %other to i32
  %a = shl i32 %value, %wide
  %b = lshr i32 %value, %other_wide
  %result = or i32 %a, %b
  ret i32 %result
}

; Every constant count stays inline at both optimization levels.

define i32 @rotl32_0(i32 %value) {
; CHECK-LABEL: rotl32_0:
; CHECK-NOT: __avm_
; CHECK: ret
  %result = call i32 @llvm.fshl.i32(i32 %value, i32 %value, i32 0)
  ret i32 %result
}

define i32 @rotl32_1(i32 %value) {
; CHECK-LABEL: rotl32_1:
; CHECK-NOT: __avm_
; CHECK: ret
  %result = call i32 @llvm.fshl.i32(i32 %value, i32 %value, i32 1)
  ret i32 %result
}

define i32 @rotl32_2(i32 %value) {
; CHECK-LABEL: rotl32_2:
; CHECK-NOT: __avm_
; CHECK: ret
  %result = call i32 @llvm.fshl.i32(i32 %value, i32 %value, i32 2)
  ret i32 %result
}

define i32 @rotl32_3(i32 %value) {
; CHECK-LABEL: rotl32_3:
; CHECK-NOT: __avm_
; CHECK: ret
  %result = call i32 @llvm.fshl.i32(i32 %value, i32 %value, i32 3)
  ret i32 %result
}

define i32 @rotl32_4(i32 %value) {
; CHECK-LABEL: rotl32_4:
; CHECK-NOT: __avm_
; CHECK: ret
  %result = call i32 @llvm.fshl.i32(i32 %value, i32 %value, i32 4)
  ret i32 %result
}

define i32 @rotl32_5(i32 %value) {
; CHECK-LABEL: rotl32_5:
; CHECK-NOT: __avm_
; CHECK: ret
  %result = call i32 @llvm.fshl.i32(i32 %value, i32 %value, i32 5)
  ret i32 %result
}

define i32 @rotl32_6(i32 %value) {
; CHECK-LABEL: rotl32_6:
; CHECK-NOT: __avm_
; CHECK: ret
  %result = call i32 @llvm.fshl.i32(i32 %value, i32 %value, i32 6)
  ret i32 %result
}

define i32 @rotl32_7(i32 %value) {
; CHECK-LABEL: rotl32_7:
; CHECK-NOT: __avm_
; CHECK: ret
  %result = call i32 @llvm.fshl.i32(i32 %value, i32 %value, i32 7)
  ret i32 %result
}

define i32 @rotl32_8(i32 %value) {
; CHECK-LABEL: rotl32_8:
; CHECK-NOT: __avm_
; CHECK: ret
  %result = call i32 @llvm.fshl.i32(i32 %value, i32 %value, i32 8)
  ret i32 %result
}

define i32 @rotl32_9(i32 %value) {
; CHECK-LABEL: rotl32_9:
; CHECK-NOT: __avm_
; CHECK: ret
  %result = call i32 @llvm.fshl.i32(i32 %value, i32 %value, i32 9)
  ret i32 %result
}

define i32 @rotl32_10(i32 %value) {
; CHECK-LABEL: rotl32_10:
; CHECK-NOT: __avm_
; CHECK: ret
  %result = call i32 @llvm.fshl.i32(i32 %value, i32 %value, i32 10)
  ret i32 %result
}

define i32 @rotl32_11(i32 %value) {
; CHECK-LABEL: rotl32_11:
; CHECK-NOT: __avm_
; CHECK: ret
  %result = call i32 @llvm.fshl.i32(i32 %value, i32 %value, i32 11)
  ret i32 %result
}

define i32 @rotl32_12(i32 %value) {
; CHECK-LABEL: rotl32_12:
; CHECK-NOT: __avm_
; CHECK: ret
  %result = call i32 @llvm.fshl.i32(i32 %value, i32 %value, i32 12)
  ret i32 %result
}

define i32 @rotl32_13(i32 %value) {
; CHECK-LABEL: rotl32_13:
; CHECK-NOT: __avm_
; CHECK: ret
  %result = call i32 @llvm.fshl.i32(i32 %value, i32 %value, i32 13)
  ret i32 %result
}

define i32 @rotl32_14(i32 %value) {
; CHECK-LABEL: rotl32_14:
; CHECK-NOT: __avm_
; CHECK: ret
  %result = call i32 @llvm.fshl.i32(i32 %value, i32 %value, i32 14)
  ret i32 %result
}

define i32 @rotl32_15(i32 %value) {
; CHECK-LABEL: rotl32_15:
; CHECK-NOT: __avm_
; CHECK: ret
  %result = call i32 @llvm.fshl.i32(i32 %value, i32 %value, i32 15)
  ret i32 %result
}

define i32 @rotl32_16(i32 %value) {
; CHECK-LABEL: rotl32_16:
; CHECK-NOT: __avm_
; CHECK: ret
  %result = call i32 @llvm.fshl.i32(i32 %value, i32 %value, i32 16)
  ret i32 %result
}

define i32 @rotl32_17(i32 %value) {
; CHECK-LABEL: rotl32_17:
; CHECK-NOT: __avm_
; CHECK: ret
  %result = call i32 @llvm.fshl.i32(i32 %value, i32 %value, i32 17)
  ret i32 %result
}

define i32 @rotl32_18(i32 %value) {
; CHECK-LABEL: rotl32_18:
; CHECK-NOT: __avm_
; CHECK: ret
  %result = call i32 @llvm.fshl.i32(i32 %value, i32 %value, i32 18)
  ret i32 %result
}

define i32 @rotl32_19(i32 %value) {
; CHECK-LABEL: rotl32_19:
; CHECK-NOT: __avm_
; CHECK: ret
  %result = call i32 @llvm.fshl.i32(i32 %value, i32 %value, i32 19)
  ret i32 %result
}

define i32 @rotl32_20(i32 %value) {
; CHECK-LABEL: rotl32_20:
; CHECK-NOT: __avm_
; CHECK: ret
  %result = call i32 @llvm.fshl.i32(i32 %value, i32 %value, i32 20)
  ret i32 %result
}

define i32 @rotl32_21(i32 %value) {
; CHECK-LABEL: rotl32_21:
; CHECK-NOT: __avm_
; CHECK: ret
  %result = call i32 @llvm.fshl.i32(i32 %value, i32 %value, i32 21)
  ret i32 %result
}

define i32 @rotl32_22(i32 %value) {
; CHECK-LABEL: rotl32_22:
; CHECK-NOT: __avm_
; CHECK: ret
  %result = call i32 @llvm.fshl.i32(i32 %value, i32 %value, i32 22)
  ret i32 %result
}

define i32 @rotl32_23(i32 %value) {
; CHECK-LABEL: rotl32_23:
; CHECK-NOT: __avm_
; CHECK: ret
  %result = call i32 @llvm.fshl.i32(i32 %value, i32 %value, i32 23)
  ret i32 %result
}

define i32 @rotl32_24(i32 %value) {
; CHECK-LABEL: rotl32_24:
; CHECK-NOT: __avm_
; CHECK: ret
  %result = call i32 @llvm.fshl.i32(i32 %value, i32 %value, i32 24)
  ret i32 %result
}

define i32 @rotl32_25(i32 %value) {
; CHECK-LABEL: rotl32_25:
; CHECK-NOT: __avm_
; CHECK: ret
  %result = call i32 @llvm.fshl.i32(i32 %value, i32 %value, i32 25)
  ret i32 %result
}

define i32 @rotl32_26(i32 %value) {
; CHECK-LABEL: rotl32_26:
; CHECK-NOT: __avm_
; CHECK: ret
  %result = call i32 @llvm.fshl.i32(i32 %value, i32 %value, i32 26)
  ret i32 %result
}

define i32 @rotl32_27(i32 %value) {
; CHECK-LABEL: rotl32_27:
; CHECK-NOT: __avm_
; CHECK: ret
  %result = call i32 @llvm.fshl.i32(i32 %value, i32 %value, i32 27)
  ret i32 %result
}

define i32 @rotl32_28(i32 %value) {
; CHECK-LABEL: rotl32_28:
; CHECK-NOT: __avm_
; CHECK: ret
  %result = call i32 @llvm.fshl.i32(i32 %value, i32 %value, i32 28)
  ret i32 %result
}

define i32 @rotl32_29(i32 %value) {
; CHECK-LABEL: rotl32_29:
; CHECK-NOT: __avm_
; CHECK: ret
  %result = call i32 @llvm.fshl.i32(i32 %value, i32 %value, i32 29)
  ret i32 %result
}

define i32 @rotl32_30(i32 %value) {
; CHECK-LABEL: rotl32_30:
; CHECK-NOT: __avm_
; CHECK: ret
  %result = call i32 @llvm.fshl.i32(i32 %value, i32 %value, i32 30)
  ret i32 %result
}

define i32 @rotl32_31(i32 %value) {
; CHECK-LABEL: rotl32_31:
; CHECK-NOT: __avm_
; CHECK: ret
  %result = call i32 @llvm.fshl.i32(i32 %value, i32 %value, i32 31)
  ret i32 %result
}

define i32 @rotl32_32(i32 %value) {
; CHECK-LABEL: rotl32_32:
; CHECK-NOT: __avm_
; CHECK: ret
  %result = call i32 @llvm.fshl.i32(i32 %value, i32 %value, i32 32)
  ret i32 %result
}

define i32 @rotl32_65552(i32 %value) {
; CHECK-LABEL: rotl32_65552:
; CHECK-NOT: __avm_
; CHECK: ret
  %result = call i32 @llvm.fshl.i32(i32 %value, i32 %value, i32 65552)
  ret i32 %result
}

define i32 @rotl32_4294967295(i32 %value) {
; CHECK-LABEL: rotl32_4294967295:
; CHECK-NOT: __avm_
; CHECK: ret
  %result = call i32 @llvm.fshl.i32(i32 %value, i32 %value, i32 4294967295)
  ret i32 %result
}

define i32 @rotr32_0(i32 %value) {
; CHECK-LABEL: rotr32_0:
; CHECK-NOT: __avm_
; CHECK: ret
  %result = call i32 @llvm.fshr.i32(i32 %value, i32 %value, i32 0)
  ret i32 %result
}

define i32 @rotr32_1(i32 %value) {
; CHECK-LABEL: rotr32_1:
; CHECK-NOT: __avm_
; CHECK: ret
  %result = call i32 @llvm.fshr.i32(i32 %value, i32 %value, i32 1)
  ret i32 %result
}

define i32 @rotr32_2(i32 %value) {
; CHECK-LABEL: rotr32_2:
; CHECK-NOT: __avm_
; CHECK: ret
  %result = call i32 @llvm.fshr.i32(i32 %value, i32 %value, i32 2)
  ret i32 %result
}

define i32 @rotr32_3(i32 %value) {
; CHECK-LABEL: rotr32_3:
; CHECK-NOT: __avm_
; CHECK: ret
  %result = call i32 @llvm.fshr.i32(i32 %value, i32 %value, i32 3)
  ret i32 %result
}

define i32 @rotr32_4(i32 %value) {
; CHECK-LABEL: rotr32_4:
; CHECK-NOT: __avm_
; CHECK: ret
  %result = call i32 @llvm.fshr.i32(i32 %value, i32 %value, i32 4)
  ret i32 %result
}

define i32 @rotr32_5(i32 %value) {
; CHECK-LABEL: rotr32_5:
; CHECK-NOT: __avm_
; CHECK: ret
  %result = call i32 @llvm.fshr.i32(i32 %value, i32 %value, i32 5)
  ret i32 %result
}

define i32 @rotr32_6(i32 %value) {
; CHECK-LABEL: rotr32_6:
; CHECK-NOT: __avm_
; CHECK: ret
  %result = call i32 @llvm.fshr.i32(i32 %value, i32 %value, i32 6)
  ret i32 %result
}

define i32 @rotr32_7(i32 %value) {
; CHECK-LABEL: rotr32_7:
; CHECK-NOT: __avm_
; CHECK: ret
  %result = call i32 @llvm.fshr.i32(i32 %value, i32 %value, i32 7)
  ret i32 %result
}

define i32 @rotr32_8(i32 %value) {
; CHECK-LABEL: rotr32_8:
; CHECK-NOT: __avm_
; CHECK: ret
  %result = call i32 @llvm.fshr.i32(i32 %value, i32 %value, i32 8)
  ret i32 %result
}

define i32 @rotr32_9(i32 %value) {
; CHECK-LABEL: rotr32_9:
; CHECK-NOT: __avm_
; CHECK: ret
  %result = call i32 @llvm.fshr.i32(i32 %value, i32 %value, i32 9)
  ret i32 %result
}

define i32 @rotr32_10(i32 %value) {
; CHECK-LABEL: rotr32_10:
; CHECK-NOT: __avm_
; CHECK: ret
  %result = call i32 @llvm.fshr.i32(i32 %value, i32 %value, i32 10)
  ret i32 %result
}

define i32 @rotr32_11(i32 %value) {
; CHECK-LABEL: rotr32_11:
; CHECK-NOT: __avm_
; CHECK: ret
  %result = call i32 @llvm.fshr.i32(i32 %value, i32 %value, i32 11)
  ret i32 %result
}

define i32 @rotr32_12(i32 %value) {
; CHECK-LABEL: rotr32_12:
; CHECK-NOT: __avm_
; CHECK: ret
  %result = call i32 @llvm.fshr.i32(i32 %value, i32 %value, i32 12)
  ret i32 %result
}

define i32 @rotr32_13(i32 %value) {
; CHECK-LABEL: rotr32_13:
; CHECK-NOT: __avm_
; CHECK: ret
  %result = call i32 @llvm.fshr.i32(i32 %value, i32 %value, i32 13)
  ret i32 %result
}

define i32 @rotr32_14(i32 %value) {
; CHECK-LABEL: rotr32_14:
; CHECK-NOT: __avm_
; CHECK: ret
  %result = call i32 @llvm.fshr.i32(i32 %value, i32 %value, i32 14)
  ret i32 %result
}

define i32 @rotr32_15(i32 %value) {
; CHECK-LABEL: rotr32_15:
; CHECK-NOT: __avm_
; CHECK: ret
  %result = call i32 @llvm.fshr.i32(i32 %value, i32 %value, i32 15)
  ret i32 %result
}

define i32 @rotr32_16(i32 %value) {
; CHECK-LABEL: rotr32_16:
; CHECK-NOT: __avm_
; CHECK: ret
  %result = call i32 @llvm.fshr.i32(i32 %value, i32 %value, i32 16)
  ret i32 %result
}

define i32 @rotr32_17(i32 %value) {
; CHECK-LABEL: rotr32_17:
; CHECK-NOT: __avm_
; CHECK: ret
  %result = call i32 @llvm.fshr.i32(i32 %value, i32 %value, i32 17)
  ret i32 %result
}

define i32 @rotr32_18(i32 %value) {
; CHECK-LABEL: rotr32_18:
; CHECK-NOT: __avm_
; CHECK: ret
  %result = call i32 @llvm.fshr.i32(i32 %value, i32 %value, i32 18)
  ret i32 %result
}

define i32 @rotr32_19(i32 %value) {
; CHECK-LABEL: rotr32_19:
; CHECK-NOT: __avm_
; CHECK: ret
  %result = call i32 @llvm.fshr.i32(i32 %value, i32 %value, i32 19)
  ret i32 %result
}

define i32 @rotr32_20(i32 %value) {
; CHECK-LABEL: rotr32_20:
; CHECK-NOT: __avm_
; CHECK: ret
  %result = call i32 @llvm.fshr.i32(i32 %value, i32 %value, i32 20)
  ret i32 %result
}

define i32 @rotr32_21(i32 %value) {
; CHECK-LABEL: rotr32_21:
; CHECK-NOT: __avm_
; CHECK: ret
  %result = call i32 @llvm.fshr.i32(i32 %value, i32 %value, i32 21)
  ret i32 %result
}

define i32 @rotr32_22(i32 %value) {
; CHECK-LABEL: rotr32_22:
; CHECK-NOT: __avm_
; CHECK: ret
  %result = call i32 @llvm.fshr.i32(i32 %value, i32 %value, i32 22)
  ret i32 %result
}

define i32 @rotr32_23(i32 %value) {
; CHECK-LABEL: rotr32_23:
; CHECK-NOT: __avm_
; CHECK: ret
  %result = call i32 @llvm.fshr.i32(i32 %value, i32 %value, i32 23)
  ret i32 %result
}

define i32 @rotr32_24(i32 %value) {
; CHECK-LABEL: rotr32_24:
; CHECK-NOT: __avm_
; CHECK: ret
  %result = call i32 @llvm.fshr.i32(i32 %value, i32 %value, i32 24)
  ret i32 %result
}

define i32 @rotr32_25(i32 %value) {
; CHECK-LABEL: rotr32_25:
; CHECK-NOT: __avm_
; CHECK: ret
  %result = call i32 @llvm.fshr.i32(i32 %value, i32 %value, i32 25)
  ret i32 %result
}

define i32 @rotr32_26(i32 %value) {
; CHECK-LABEL: rotr32_26:
; CHECK-NOT: __avm_
; CHECK: ret
  %result = call i32 @llvm.fshr.i32(i32 %value, i32 %value, i32 26)
  ret i32 %result
}

define i32 @rotr32_27(i32 %value) {
; CHECK-LABEL: rotr32_27:
; CHECK-NOT: __avm_
; CHECK: ret
  %result = call i32 @llvm.fshr.i32(i32 %value, i32 %value, i32 27)
  ret i32 %result
}

define i32 @rotr32_28(i32 %value) {
; CHECK-LABEL: rotr32_28:
; CHECK-NOT: __avm_
; CHECK: ret
  %result = call i32 @llvm.fshr.i32(i32 %value, i32 %value, i32 28)
  ret i32 %result
}

define i32 @rotr32_29(i32 %value) {
; CHECK-LABEL: rotr32_29:
; CHECK-NOT: __avm_
; CHECK: ret
  %result = call i32 @llvm.fshr.i32(i32 %value, i32 %value, i32 29)
  ret i32 %result
}

define i32 @rotr32_30(i32 %value) {
; CHECK-LABEL: rotr32_30:
; CHECK-NOT: __avm_
; CHECK: ret
  %result = call i32 @llvm.fshr.i32(i32 %value, i32 %value, i32 30)
  ret i32 %result
}

define i32 @rotr32_31(i32 %value) {
; CHECK-LABEL: rotr32_31:
; CHECK-NOT: __avm_
; CHECK: ret
  %result = call i32 @llvm.fshr.i32(i32 %value, i32 %value, i32 31)
  ret i32 %result
}

define i32 @rotr32_32(i32 %value) {
; CHECK-LABEL: rotr32_32:
; CHECK-NOT: __avm_
; CHECK: ret
  %result = call i32 @llvm.fshr.i32(i32 %value, i32 %value, i32 32)
  ret i32 %result
}

define i32 @rotr32_65552(i32 %value) {
; CHECK-LABEL: rotr32_65552:
; CHECK-NOT: __avm_
; CHECK: ret
  %result = call i32 @llvm.fshr.i32(i32 %value, i32 %value, i32 65552)
  ret i32 %result
}

define i32 @rotr32_4294967295(i32 %value) {
; CHECK-LABEL: rotr32_4294967295:
; CHECK-NOT: __avm_
; CHECK: ret
  %result = call i32 @llvm.fshr.i32(i32 %value, i32 %value, i32 4294967295)
  ret i32 %result
}

; A funnel shift with different input values is not a rotation.
define i32 @fshl32(i32 %a, i32 %b, i32 %count) {
; CHECK-LABEL: fshl32:
; CHECK-NOT: __avm_rot
; CHECK: ret
  %result = call i32 @llvm.fshl.i32(i32 %a, i32 %b, i32 %count)
  ret i32 %result
}

; A funnel shift with different input values is not a rotation.
define i32 @fshr32(i32 %a, i32 %b, i32 %count) {
; CHECK-LABEL: fshr32:
; CHECK-NOT: __avm_rot
; CHECK: ret
  %result = call i32 @llvm.fshr.i32(i32 %a, i32 %b, i32 %count)
  ret i32 %result
}
