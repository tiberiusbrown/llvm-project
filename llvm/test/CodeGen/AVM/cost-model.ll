; RUN: opt -mtriple=avm-unknown-arduboyfx -mcpu=avm1 \
; RUN:   -mtune=avm-interpreter-32u4-v1 -passes="print<cost-model>" \
; RUN:   -cost-kind=throughput -disable-output 2>&1 < %s | FileCheck %s

define void @scalar_costs(i16 %a, i16 %b, i16 %count) {
; CHECK-LABEL: Cost Model: Found an estimated cost of 13 for instruction: %udiv = udiv i16 %a, %b
; CHECK:       Cost Model: Found an estimated cost of 13 for instruction: %urem = urem i16 %a, %b
; CHECK:       Cost Model: Found an estimated cost of 13 for instruction: %sdiv = sdiv i16 %a, %b
; CHECK:       Cost Model: Found an estimated cost of 13 for instruction: %srem = srem i16 %a, %b
; CHECK:       Cost Model: Found an estimated cost of 1 for instruction: %shl1 = shl i16 %a, 1
; CHECK:       Cost Model: Found an estimated cost of 3 for instruction: %shl3 = shl i16 %a, 3
; CHECK:       Cost Model: Found an estimated cost of 4 for instruction: %shl4 = shl i16 %a, 4
; CHECK:       Cost Model: Found an estimated cost of 2 for instruction: %lshr1 = lshr i16 %a, 1
; CHECK:       Cost Model: Found an estimated cost of 3 for instruction: %lshr2 = lshr i16 %a, 2
; CHECK:       Cost Model: Found an estimated cost of 5 for instruction: %ashr15 = ashr i16 %a, 15
; CHECK:       Cost Model: Found an estimated cost of 4 for instruction: %variable = shl i16 %a, %count
  %udiv = udiv i16 %a, %b
  %urem = urem i16 %a, %b
  %sdiv = sdiv i16 %a, %b
  %srem = srem i16 %a, %b
  %shl1 = shl i16 %a, 1
  %shl3 = shl i16 %a, 3
  %shl4 = shl i16 %a, 4
  %lshr1 = lshr i16 %a, 1
  %lshr2 = lshr i16 %a, 2
  %ashr15 = ashr i16 %a, 15
  %variable = shl i16 %a, %count
  ret void
}
