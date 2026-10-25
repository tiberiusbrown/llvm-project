; RUN: llc -mtriple=avm -O2 -verify-machineinstrs %s -o - | FileCheck %s

; Sixteen hoisted byte coefficients fill more than the register file. The
; reduction must consume each product instead of spilling independent products
; until the accumulator becomes ready. Coefficient spills in entry are expected.
; Scheduling must preserve the arithmetic graph (including wrapping sums).

define i16 @fir_signed_bytes(ptr %samples, ptr %coefficients, i16 %count) {
; CHECK-LABEL: fir_signed_bytes:
; CHECK: %entry
; CHECK: stsp16
; CHECK: .LBB0_1:
; CHECK-NOT: stsp
; CHECK: muls8.w
; CHECK-NOT: stsp
; CHECK: ret
entry:
  %cp0 = getelementptr i8, ptr %coefficients, i16 0
  %c0 = load i8, ptr %cp0, align 1
  %cw0 = sext i8 %c0 to i16
  %cp1 = getelementptr i8, ptr %coefficients, i16 1
  %c1 = load i8, ptr %cp1, align 1
  %cw1 = sext i8 %c1 to i16
  %cp2 = getelementptr i8, ptr %coefficients, i16 2
  %c2 = load i8, ptr %cp2, align 1
  %cw2 = sext i8 %c2 to i16
  %cp3 = getelementptr i8, ptr %coefficients, i16 3
  %c3 = load i8, ptr %cp3, align 1
  %cw3 = sext i8 %c3 to i16
  %cp4 = getelementptr i8, ptr %coefficients, i16 4
  %c4 = load i8, ptr %cp4, align 1
  %cw4 = sext i8 %c4 to i16
  %cp5 = getelementptr i8, ptr %coefficients, i16 5
  %c5 = load i8, ptr %cp5, align 1
  %cw5 = sext i8 %c5 to i16
  %cp6 = getelementptr i8, ptr %coefficients, i16 6
  %c6 = load i8, ptr %cp6, align 1
  %cw6 = sext i8 %c6 to i16
  %cp7 = getelementptr i8, ptr %coefficients, i16 7
  %c7 = load i8, ptr %cp7, align 1
  %cw7 = sext i8 %c7 to i16
  %cp8 = getelementptr i8, ptr %coefficients, i16 8
  %c8 = load i8, ptr %cp8, align 1
  %cw8 = sext i8 %c8 to i16
  %cp9 = getelementptr i8, ptr %coefficients, i16 9
  %c9 = load i8, ptr %cp9, align 1
  %cw9 = sext i8 %c9 to i16
  %cp10 = getelementptr i8, ptr %coefficients, i16 10
  %c10 = load i8, ptr %cp10, align 1
  %cw10 = sext i8 %c10 to i16
  %cp11 = getelementptr i8, ptr %coefficients, i16 11
  %c11 = load i8, ptr %cp11, align 1
  %cw11 = sext i8 %c11 to i16
  %cp12 = getelementptr i8, ptr %coefficients, i16 12
  %c12 = load i8, ptr %cp12, align 1
  %cw12 = sext i8 %c12 to i16
  %cp13 = getelementptr i8, ptr %coefficients, i16 13
  %c13 = load i8, ptr %cp13, align 1
  %cw13 = sext i8 %c13 to i16
  %cp14 = getelementptr i8, ptr %coefficients, i16 14
  %c14 = load i8, ptr %cp14, align 1
  %cw14 = sext i8 %c14 to i16
  %cp15 = getelementptr i8, ptr %coefficients, i16 15
  %c15 = load i8, ptr %cp15, align 1
  %cw15 = sext i8 %c15 to i16
  br label %loop

loop:
  %i = phi i16 [ 0, %entry ], [ %next, %loop ]
  %checksum = phi i16 [ 0, %entry ], [ %result, %loop ]
  %base = getelementptr i8, ptr %samples, i16 %i
  %p0 = getelementptr i8, ptr %base, i16 0
  %s0 = load i8, ptr %p0, align 1
  %sw0 = sext i8 %s0 to i16
  %product0 = mul nsw i16 %sw0, %cw0
  %p1 = getelementptr i8, ptr %base, i16 1
  %s1 = load i8, ptr %p1, align 1
  %sw1 = sext i8 %s1 to i16
  %product1 = mul nsw i16 %sw1, %cw1
  %sum1 = add i16 %product0, %product1
  %p2 = getelementptr i8, ptr %base, i16 2
  %s2 = load i8, ptr %p2, align 1
  %sw2 = sext i8 %s2 to i16
  %product2 = mul nsw i16 %sw2, %cw2
  %sum2 = add i16 %sum1, %product2
  %p3 = getelementptr i8, ptr %base, i16 3
  %s3 = load i8, ptr %p3, align 1
  %sw3 = sext i8 %s3 to i16
  %product3 = mul nsw i16 %sw3, %cw3
  %sum3 = add i16 %sum2, %product3
  %p4 = getelementptr i8, ptr %base, i16 4
  %s4 = load i8, ptr %p4, align 1
  %sw4 = sext i8 %s4 to i16
  %product4 = mul nsw i16 %sw4, %cw4
  %sum4 = add i16 %sum3, %product4
  %p5 = getelementptr i8, ptr %base, i16 5
  %s5 = load i8, ptr %p5, align 1
  %sw5 = sext i8 %s5 to i16
  %product5 = mul nsw i16 %sw5, %cw5
  %sum5 = add i16 %sum4, %product5
  %p6 = getelementptr i8, ptr %base, i16 6
  %s6 = load i8, ptr %p6, align 1
  %sw6 = sext i8 %s6 to i16
  %product6 = mul nsw i16 %sw6, %cw6
  %sum6 = add i16 %sum5, %product6
  %p7 = getelementptr i8, ptr %base, i16 7
  %s7 = load i8, ptr %p7, align 1
  %sw7 = sext i8 %s7 to i16
  %product7 = mul nsw i16 %sw7, %cw7
  %sum7 = add i16 %sum6, %product7
  %p8 = getelementptr i8, ptr %base, i16 8
  %s8 = load i8, ptr %p8, align 1
  %sw8 = sext i8 %s8 to i16
  %product8 = mul nsw i16 %sw8, %cw8
  %sum8 = add i16 %sum7, %product8
  %p9 = getelementptr i8, ptr %base, i16 9
  %s9 = load i8, ptr %p9, align 1
  %sw9 = sext i8 %s9 to i16
  %product9 = mul nsw i16 %sw9, %cw9
  %sum9 = add i16 %sum8, %product9
  %p10 = getelementptr i8, ptr %base, i16 10
  %s10 = load i8, ptr %p10, align 1
  %sw10 = sext i8 %s10 to i16
  %product10 = mul nsw i16 %sw10, %cw10
  %sum10 = add i16 %sum9, %product10
  %p11 = getelementptr i8, ptr %base, i16 11
  %s11 = load i8, ptr %p11, align 1
  %sw11 = sext i8 %s11 to i16
  %product11 = mul nsw i16 %sw11, %cw11
  %sum11 = add i16 %sum10, %product11
  %p12 = getelementptr i8, ptr %base, i16 12
  %s12 = load i8, ptr %p12, align 1
  %sw12 = sext i8 %s12 to i16
  %product12 = mul nsw i16 %sw12, %cw12
  %sum12 = add i16 %sum11, %product12
  %p13 = getelementptr i8, ptr %base, i16 13
  %s13 = load i8, ptr %p13, align 1
  %sw13 = sext i8 %s13 to i16
  %product13 = mul nsw i16 %sw13, %cw13
  %sum13 = add i16 %sum12, %product13
  %p14 = getelementptr i8, ptr %base, i16 14
  %s14 = load i8, ptr %p14, align 1
  %sw14 = sext i8 %s14 to i16
  %product14 = mul nsw i16 %sw14, %cw14
  %sum14 = add i16 %sum13, %product14
  %p15 = getelementptr i8, ptr %base, i16 15
  %s15 = load i8, ptr %p15, align 1
  %sw15 = sext i8 %s15 to i16
  %product15 = mul nsw i16 %sw15, %cw15
  %sum15 = add i16 %sum14, %product15
  %result = add i16 %checksum, %sum15
  %next = add i16 %i, 1
  %done = icmp eq i16 %next, %count
  br i1 %done, label %exit, label %loop

exit:
  ret i16 %result
}
; The same policy applies to bitwise reductions and unsigned widening products.
define i16 @xor_unsigned_products(ptr %samples, ptr %coefficients, i16 %count) {
; CHECK-LABEL: xor_unsigned_products:
; CHECK: %entry
; CHECK: stsp16
; CHECK: .LBB1_1:
; CHECK-NOT: stsp
; CHECK: mulu8.w
; CHECK-NOT: stsp
; CHECK: ret
entry:
  %cp0 = getelementptr i8, ptr %coefficients, i16 0
  %c0 = load i8, ptr %cp0, align 1
  %cw0 = zext i8 %c0 to i16
  %cp1 = getelementptr i8, ptr %coefficients, i16 1
  %c1 = load i8, ptr %cp1, align 1
  %cw1 = zext i8 %c1 to i16
  %cp2 = getelementptr i8, ptr %coefficients, i16 2
  %c2 = load i8, ptr %cp2, align 1
  %cw2 = zext i8 %c2 to i16
  %cp3 = getelementptr i8, ptr %coefficients, i16 3
  %c3 = load i8, ptr %cp3, align 1
  %cw3 = zext i8 %c3 to i16
  %cp4 = getelementptr i8, ptr %coefficients, i16 4
  %c4 = load i8, ptr %cp4, align 1
  %cw4 = zext i8 %c4 to i16
  %cp5 = getelementptr i8, ptr %coefficients, i16 5
  %c5 = load i8, ptr %cp5, align 1
  %cw5 = zext i8 %c5 to i16
  %cp6 = getelementptr i8, ptr %coefficients, i16 6
  %c6 = load i8, ptr %cp6, align 1
  %cw6 = zext i8 %c6 to i16
  %cp7 = getelementptr i8, ptr %coefficients, i16 7
  %c7 = load i8, ptr %cp7, align 1
  %cw7 = zext i8 %c7 to i16
  %cp8 = getelementptr i8, ptr %coefficients, i16 8
  %c8 = load i8, ptr %cp8, align 1
  %cw8 = zext i8 %c8 to i16
  %cp9 = getelementptr i8, ptr %coefficients, i16 9
  %c9 = load i8, ptr %cp9, align 1
  %cw9 = zext i8 %c9 to i16
  %cp10 = getelementptr i8, ptr %coefficients, i16 10
  %c10 = load i8, ptr %cp10, align 1
  %cw10 = zext i8 %c10 to i16
  %cp11 = getelementptr i8, ptr %coefficients, i16 11
  %c11 = load i8, ptr %cp11, align 1
  %cw11 = zext i8 %c11 to i16
  %cp12 = getelementptr i8, ptr %coefficients, i16 12
  %c12 = load i8, ptr %cp12, align 1
  %cw12 = zext i8 %c12 to i16
  %cp13 = getelementptr i8, ptr %coefficients, i16 13
  %c13 = load i8, ptr %cp13, align 1
  %cw13 = zext i8 %c13 to i16
  %cp14 = getelementptr i8, ptr %coefficients, i16 14
  %c14 = load i8, ptr %cp14, align 1
  %cw14 = zext i8 %c14 to i16
  %cp15 = getelementptr i8, ptr %coefficients, i16 15
  %c15 = load i8, ptr %cp15, align 1
  %cw15 = zext i8 %c15 to i16
  br label %loop

loop:
  %i = phi i16 [ 0, %entry ], [ %next, %loop ]
  %checksum = phi i16 [ 0, %entry ], [ %result, %loop ]
  %base = getelementptr i8, ptr %samples, i16 %i
  %p0 = getelementptr i8, ptr %base, i16 0
  %s0 = load i8, ptr %p0, align 1
  %sw0 = zext i8 %s0 to i16
  %product0 = mul nuw i16 %sw0, %cw0
  %p1 = getelementptr i8, ptr %base, i16 1
  %s1 = load i8, ptr %p1, align 1
  %sw1 = zext i8 %s1 to i16
  %product1 = mul nuw i16 %sw1, %cw1
  %sum1 = xor i16 %product0, %product1
  %p2 = getelementptr i8, ptr %base, i16 2
  %s2 = load i8, ptr %p2, align 1
  %sw2 = zext i8 %s2 to i16
  %product2 = mul nuw i16 %sw2, %cw2
  %sum2 = xor i16 %sum1, %product2
  %p3 = getelementptr i8, ptr %base, i16 3
  %s3 = load i8, ptr %p3, align 1
  %sw3 = zext i8 %s3 to i16
  %product3 = mul nuw i16 %sw3, %cw3
  %sum3 = xor i16 %sum2, %product3
  %p4 = getelementptr i8, ptr %base, i16 4
  %s4 = load i8, ptr %p4, align 1
  %sw4 = zext i8 %s4 to i16
  %product4 = mul nuw i16 %sw4, %cw4
  %sum4 = xor i16 %sum3, %product4
  %p5 = getelementptr i8, ptr %base, i16 5
  %s5 = load i8, ptr %p5, align 1
  %sw5 = zext i8 %s5 to i16
  %product5 = mul nuw i16 %sw5, %cw5
  %sum5 = xor i16 %sum4, %product5
  %p6 = getelementptr i8, ptr %base, i16 6
  %s6 = load i8, ptr %p6, align 1
  %sw6 = zext i8 %s6 to i16
  %product6 = mul nuw i16 %sw6, %cw6
  %sum6 = xor i16 %sum5, %product6
  %p7 = getelementptr i8, ptr %base, i16 7
  %s7 = load i8, ptr %p7, align 1
  %sw7 = zext i8 %s7 to i16
  %product7 = mul nuw i16 %sw7, %cw7
  %sum7 = xor i16 %sum6, %product7
  %p8 = getelementptr i8, ptr %base, i16 8
  %s8 = load i8, ptr %p8, align 1
  %sw8 = zext i8 %s8 to i16
  %product8 = mul nuw i16 %sw8, %cw8
  %sum8 = xor i16 %sum7, %product8
  %p9 = getelementptr i8, ptr %base, i16 9
  %s9 = load i8, ptr %p9, align 1
  %sw9 = zext i8 %s9 to i16
  %product9 = mul nuw i16 %sw9, %cw9
  %sum9 = xor i16 %sum8, %product9
  %p10 = getelementptr i8, ptr %base, i16 10
  %s10 = load i8, ptr %p10, align 1
  %sw10 = zext i8 %s10 to i16
  %product10 = mul nuw i16 %sw10, %cw10
  %sum10 = xor i16 %sum9, %product10
  %p11 = getelementptr i8, ptr %base, i16 11
  %s11 = load i8, ptr %p11, align 1
  %sw11 = zext i8 %s11 to i16
  %product11 = mul nuw i16 %sw11, %cw11
  %sum11 = xor i16 %sum10, %product11
  %p12 = getelementptr i8, ptr %base, i16 12
  %s12 = load i8, ptr %p12, align 1
  %sw12 = zext i8 %s12 to i16
  %product12 = mul nuw i16 %sw12, %cw12
  %sum12 = xor i16 %sum11, %product12
  %p13 = getelementptr i8, ptr %base, i16 13
  %s13 = load i8, ptr %p13, align 1
  %sw13 = zext i8 %s13 to i16
  %product13 = mul nuw i16 %sw13, %cw13
  %sum13 = xor i16 %sum12, %product13
  %p14 = getelementptr i8, ptr %base, i16 14
  %s14 = load i8, ptr %p14, align 1
  %sw14 = zext i8 %s14 to i16
  %product14 = mul nuw i16 %sw14, %cw14
  %sum14 = xor i16 %sum13, %product14
  %p15 = getelementptr i8, ptr %base, i16 15
  %s15 = load i8, ptr %p15, align 1
  %sw15 = zext i8 %s15 to i16
  %product15 = mul nuw i16 %sw15, %cw15
  %sum15 = xor i16 %sum14, %product15
  %result = xor i16 %checksum, %sum15
  %next = add i16 %i, 1
  %done = icmp eq i16 %next, %count
  br i1 %done, label %exit, label %loop

exit:
  ret i16 %result
}
