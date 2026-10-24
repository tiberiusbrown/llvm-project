; RUN: llc -mtriple=avm -O2 -verify-machineinstrs < %s | FileCheck %s

declare void @consume(i24)

; The ABI promotes a three-byte aggregate to i32. Only its low byte is
; demanded by the comparison, while the other fields must survive the branch.
define void @aggregate_area(i24 %coord) {
; CHECK-LABEL: aggregate_area:
; CHECK-NOT: cmp32
; CHECK: zext8
; CHECK-NEXT: cmpi.s8 {{r[0-7]}}, 1
; CHECK: brne
; CHECK-NOT: cmp32
; CHECK: ret
  %area = and i24 %coord, 255
  %eq = icmp eq i24 %area, 1
  br i1 %eq, label %yes, label %no
yes:
  call void @consume(i24 %coord)
  ret void
no:
  ret void
}

define i16 @masked_eq(i32 %value) {
; CHECK-LABEL: masked_eq:
; CHECK-NOT: cmp32
; CHECK: zext8
; CHECK: cmpi.s8 {{r[0-7]}}, 1
; CHECK: cset.eq
  %byte = and i32 %value, 255
  %eq = icmp eq i32 %byte, 1
  %result = zext i1 %eq to i16
  ret i16 %result
}

define i16 @masked_zero(i32 %value) {
; CHECK-LABEL: masked_zero:
; CHECK-NOT: cmp32
; CHECK-NOT: zext8
; CHECK: tst8
; CHECK: cset.eq
  %byte = and i32 %value, 255
  %eq = icmp eq i32 %byte, 0
  %result = zext i1 %eq to i16
  ret i16 %result
}

define i32 @masked_select(i32 %value, i32 %yes, i32 %no) {
; CHECK-LABEL: masked_select:
; CHECK-NOT: cmp32
; CHECK: cmpi.s8 {{r[0-7]}}, 1
; CHECK: cmov
  %byte = and i32 %value, 255
  %eq = icmp eq i32 %byte, 1
  %result = select i1 %eq, i32 %yes, i32 %no
  ret i32 %result
}

; Signed i32 ordering of zero-extended words must become unsigned i16
; ordering: 32768 is positive before truncation and negative afterwards.
define i16 @unsigned_words_signed_less(i32 %a, i32 %b) {
; CHECK-LABEL: unsigned_words_signed_less:
; CHECK-NOT: cmp32
; CHECK: cmp
; CHECK: cset.ult
  %x = and i32 %a, 65535
  %y = and i32 %b, 65535
  %cmp = icmp slt i32 %x, %y
  %result = zext i1 %cmp to i16
  ret i16 %result
}

; A common nonzero high word can also be discarded for ordering.
define i16 @common_prefix_signed_less(i32 %a, i32 %b) {
; CHECK-LABEL: common_prefix_signed_less:
; CHECK-NOT: cmp32
; CHECK: cmp
; CHECK: cset.ult
  %x = and i32 %a, 65535
  %y = and i32 %b, 65535
  %left = or i32 %x, -65536
  %right = or i32 %y, -65536
  %cmp = icmp slt i32 %left, %right
  %result = zext i1 %cmp to i16
  ret i16 %result
}

define i16 @signed_words_less(i16 %a, i16 %b) {
; CHECK-LABEL: signed_words_less:
; CHECK-NOT: cmp32
; CHECK: cmp
; CHECK: cset.slt
  %x = sext i16 %a to i32
  %y = sext i16 %b to i32
  %cmp = icmp slt i32 %x, %y
  %result = zext i1 %cmp to i16
  ret i16 %result
}

define i16 @signed_words_unsigned_less(i16 %a, i16 %b) {
; CHECK-LABEL: signed_words_unsigned_less:
; CHECK-NOT: cmp32
; CHECK: cmp
; CHECK: cset.ult
  %x = sext i16 %a to i32
  %y = sext i16 %b to i32
  %cmp = icmp ult i32 %x, %y
  %result = zext i1 %cmp to i16
  ret i16 %result
}

; Unknown or mismatched upper words must still participate in the comparison.
define i16 @wide_eq(i32 %a, i32 %b) {
; CHECK-LABEL: wide_eq:
; CHECK: cmp32
  %cmp = icmp eq i32 %a, %b
  %result = zext i1 %cmp to i16
  ret i16 %result
}

define i16 @different_prefixes(i32 %a, i32 %b) {
; CHECK-LABEL: different_prefixes:
; CHECK: cmp32
  %left = and i32 %a, 131071
  %right = and i32 %b, 65535
  %cmp = icmp ult i32 %left, %right
  %result = zext i1 %cmp to i16
  ret i16 %result
}

define i16 @mixed_extensions(i16 %a, i16 %b) {
; CHECK-LABEL: mixed_extensions:
; CHECK: cmp32
  %left = sext i16 %a to i32
  %right = zext i16 %b to i32
  %cmp = icmp slt i32 %left, %right
  %result = zext i1 %cmp to i16
  ret i16 %result
}
