; RUN: llc -mtriple=avm-unknown-arduboyfx -mcpu=avm1 \
; RUN:   -mtune=avm-interpreter-32u4-v1 -O0 -verify-machineinstrs < %s \
; RUN:   | FileCheck %s --check-prefix=O0
; RUN: llc -mtriple=avm-unknown-arduboyfx -mcpu=avm1 \
; RUN:   -mtune=avm-interpreter-32u4-v1 -O2 -verify-machineinstrs < %s \
; RUN:   | FileCheck %s --check-prefixes=O2,SIZE

define i16 @inc(i16 %x) {
; O0-LABEL: inc:
; O0:       inc16
; O2-LABEL: inc:
; O2:       inc16 r4
  %r = add i16 %x, 1
  ret i16 %r
}

define i16 @dec(i16 %x) {
; O2-LABEL: dec:
; O2:       dec16 r4
  %r = sub i16 %x, 1
  ret i16 %r
}

define i16 @addi(i16 %x) {
; O2-LABEL: addi:
; O2:       addi.s8 r4, 42
  %r = add i16 %x, 42
  ret i16 %r
}

define i16 @mul8(i8 %a, i8 %b) {
; O0-LABEL: mul8:
; O0:       mul8
; O2-LABEL: mul8:
; O2:       mul8 r4, r5
  %r = mul i8 %a, %b
  %z = zext i8 %r to i16
  ret i16 %z
}

define i16 @mulu8w(i8 %a, i8 %b) {
; O2-LABEL: mulu8w:
; O2:       mulu8.w r4, r5
  %x = zext i8 %a to i16
  %y = zext i8 %b to i16
  %r = mul i16 %x, %y
  ret i16 %r
}

define i16 @muls8w(i8 %a, i8 %b) {
; O2-LABEL: muls8w:
; O2:       muls8.w r4, r5
  %x = sext i8 %a to i16
  %y = sext i8 %b to i16
  %r = mul i16 %x, %y
  ret i16 %r
}

define i16 @mulsu8w(i8 %a, i8 %b) {
; O2-LABEL: mulsu8w:
; O2:       mulsu8.w r4, r5
  %x = sext i8 %a to i16
  %y = zext i8 %b to i16
  %r = mul i16 %x, %y
  ret i16 %r
}

define i16 @mul16(i16 %a, i16 %b) {
; O2-LABEL: mul16:
; O2:       mul16 r4, r5
  %r = mul i16 %a, %b
  ret i16 %r
}

define i16 @udiv16(i16 %a, i16 %b) {
; O0-LABEL: udiv16:
; O0:       udiv16
; O2-LABEL: udiv16:
; O2:       udiv16 r4, r5
  %r = udiv i16 %a, %b
  ret i16 %r
}

define i16 @urem16(i16 %a, i16 %b) {
; O2-LABEL: urem16:
; O2:       urem16 r4, r5
  %r = urem i16 %a, %b
  ret i16 %r
}

define i16 @sdiv16(i16 %a, i16 %b) {
; O2-LABEL: sdiv16:
; O2:       sdiv16 r4, r5
  %r = sdiv i16 %a, %b
  ret i16 %r
}

define i16 @srem16(i16 %a, i16 %b) {
; O2-LABEL: srem16:
; O2:       srem16 r4, r5
  %r = srem i16 %a, %b
  ret i16 %r
}

define i16 @udiv_pow2(i16 %a) {
; O2-LABEL: udiv_pow2:
; O2:       lsr16i r4, 3
; O2-NOT:   udiv16
  %r = udiv i16 %a, 8
  ret i16 %r
}

define i16 @urem_pow2(i16 %a) {
; O2-LABEL: urem_pow2:
; O2:       and
; O2-NOT:   urem16
  %r = urem i16 %a, 8
  ret i16 %r
}

define i16 @shl0(i16 %x) {
; O2-LABEL: shl0:
; O2-NOT:   lsl
; O2:       ret
  %r = shl i16 %x, 0
  ret i16 %r
}

define i16 @shl1(i16 %x) {
; O2-LABEL: shl1:
; O2:       add r4, r4
; O2-NEXT:  ret
  %r = shl i16 %x, 1
  ret i16 %r
}

define i16 @shl2(i16 %x) {
; O2-LABEL: shl2:
; O2:       add r4, r4
; O2-NEXT:  add r4, r4
; O2-NEXT:  ret
  %r = shl i16 %x, 2
  ret i16 %r
}

define i16 @shl3(i16 %x) {
; O2-LABEL: shl3:
; O2:       add r4, r4
; O2-NEXT:  add r4, r4
; O2-NEXT:  add r4, r4
; O2-NEXT:  ret
  %r = shl i16 %x, 3
  ret i16 %r
}

define i16 @shl4(i16 %x) { ; O2-LABEL: shl4:
; O2: lsl16i r4, 4
  %r = shl i16 %x, 4
  ret i16 %r
}
define i16 @shl5(i16 %x) { ; O2-LABEL: shl5:
; O2: lsl16i r4, 5
  %r = shl i16 %x, 5
  ret i16 %r
}
define i16 @shl6(i16 %x) { ; O2-LABEL: shl6:
; O2: lsl16i r4, 6
  %r = shl i16 %x, 6
  ret i16 %r
}
define i16 @shl7(i16 %x) { ; O2-LABEL: shl7:
; O2: lsl16i r4, 7
  %r = shl i16 %x, 7
  ret i16 %r
}
define i16 @shl8(i16 %x) { ; O2-LABEL: shl8:
; O2: lsl16i r4, 8
  %r = shl i16 %x, 8
  ret i16 %r
}
define i16 @shl9(i16 %x) { ; O2-LABEL: shl9:
; O2: lsl16i r4, 9
  %r = shl i16 %x, 9
  ret i16 %r
}
define i16 @shl10(i16 %x) { ; O2-LABEL: shl10:
; O2: lsl16i r4, 10
  %r = shl i16 %x, 10
  ret i16 %r
}
define i16 @shl11(i16 %x) { ; O2-LABEL: shl11:
; O2: lsl16i r4, 11
  %r = shl i16 %x, 11
  ret i16 %r
}
define i16 @shl12(i16 %x) { ; O2-LABEL: shl12:
; O2: lsl16i r4, 12
  %r = shl i16 %x, 12
  ret i16 %r
}
define i16 @shl13(i16 %x) { ; O2-LABEL: shl13:
; O2: lsl16i r4, 13
  %r = shl i16 %x, 13
  ret i16 %r
}
define i16 @shl14(i16 %x) { ; O2-LABEL: shl14:
; O2: lsl16i r4, 14
  %r = shl i16 %x, 14
  ret i16 %r
}
define i16 @shl15(i16 %x) { ; O2-LABEL: shl15:
; O2: lsl16i r4, 15
  %r = shl i16 %x, 15
  ret i16 %r
}

define i16 @lshr0(i16 %x) { ; O2-LABEL: lshr0:
; O2-NOT: lsr
; O2: ret
  %r = lshr i16 %x, 0
  ret i16 %r
}
define i16 @lshr1(i16 %x) { ; O2-LABEL: lshr1:
; O2: lsr16.1 r4
  %r = lshr i16 %x, 1
  ret i16 %r
}
define i16 @lshr2(i16 %x) { ; O2-LABEL: lshr2:
; O2: lsr16i r4, 2
  %r = lshr i16 %x, 2
  ret i16 %r
}
define i16 @lshr3(i16 %x) { ; O2-LABEL: lshr3:
; O2: lsr16i r4, 3
  %r = lshr i16 %x, 3
  ret i16 %r
}
define i16 @lshr4(i16 %x) { ; O2-LABEL: lshr4:
; O2: lsr16i r4, 4
  %r = lshr i16 %x, 4
  ret i16 %r
}
define i16 @lshr5(i16 %x) { ; O2-LABEL: lshr5:
; O2: lsr16i r4, 5
  %r = lshr i16 %x, 5
  ret i16 %r
}
define i16 @lshr6(i16 %x) { ; O2-LABEL: lshr6:
; O2: lsr16i r4, 6
  %r = lshr i16 %x, 6
  ret i16 %r
}
define i16 @lshr7(i16 %x) { ; O2-LABEL: lshr7:
; O2: lsr16i r4, 7
  %r = lshr i16 %x, 7
  ret i16 %r
}
define i16 @lshr8(i16 %x) { ; O2-LABEL: lshr8:
; O2: lsr16i r4, 8
  %r = lshr i16 %x, 8
  ret i16 %r
}
define i16 @lshr9(i16 %x) { ; O2-LABEL: lshr9:
; O2: lsr16i r4, 9
  %r = lshr i16 %x, 9
  ret i16 %r
}
define i16 @lshr10(i16 %x) { ; O2-LABEL: lshr10:
; O2: lsr16i r4, 10
  %r = lshr i16 %x, 10
  ret i16 %r
}
define i16 @lshr11(i16 %x) { ; O2-LABEL: lshr11:
; O2: lsr16i r4, 11
  %r = lshr i16 %x, 11
  ret i16 %r
}
define i16 @lshr12(i16 %x) { ; O2-LABEL: lshr12:
; O2: lsr16i r4, 12
  %r = lshr i16 %x, 12
  ret i16 %r
}
define i16 @lshr13(i16 %x) { ; O2-LABEL: lshr13:
; O2: lsr16i r4, 13
  %r = lshr i16 %x, 13
  ret i16 %r
}
define i16 @lshr14(i16 %x) { ; O2-LABEL: lshr14:
; O2: lsr16i r4, 14
  %r = lshr i16 %x, 14
  ret i16 %r
}
define i16 @lshr15(i16 %x) { ; O2-LABEL: lshr15:
; O2: lsr16i r4, 15
  %r = lshr i16 %x, 15
  ret i16 %r
}

define i16 @ashr0(i16 %x) { ; O2-LABEL: ashr0:
; O2-NOT: asr
; O2: ret
  %r = ashr i16 %x, 0
  ret i16 %r
}
define i16 @ashr1(i16 %x) { ; O2-LABEL: ashr1:
; O2: asr16.1 r4
  %r = ashr i16 %x, 1
  ret i16 %r
}
define i16 @ashr2(i16 %x) { ; O2-LABEL: ashr2:
; O2: asr16i r4, 2
  %r = ashr i16 %x, 2
  ret i16 %r
}
define i16 @ashr3(i16 %x) { ; O2-LABEL: ashr3:
; O2: asr16i r4, 3
  %r = ashr i16 %x, 3
  ret i16 %r
}
define i16 @ashr4(i16 %x) { ; O2-LABEL: ashr4:
; O2: asr16i r4, 4
  %r = ashr i16 %x, 4
  ret i16 %r
}
define i16 @ashr5(i16 %x) { ; O2-LABEL: ashr5:
; O2: asr16i r4, 5
  %r = ashr i16 %x, 5
  ret i16 %r
}
define i16 @ashr6(i16 %x) { ; O2-LABEL: ashr6:
; O2: asr16i r4, 6
  %r = ashr i16 %x, 6
  ret i16 %r
}
define i16 @ashr7(i16 %x) { ; O2-LABEL: ashr7:
; O2: asr16i r4, 7
  %r = ashr i16 %x, 7
  ret i16 %r
}
define i16 @ashr8(i16 %x) { ; O2-LABEL: ashr8:
; O2: asr16i r4, 8
  %r = ashr i16 %x, 8
  ret i16 %r
}
define i16 @ashr9(i16 %x) { ; O2-LABEL: ashr9:
; O2: asr16i r4, 9
  %r = ashr i16 %x, 9
  ret i16 %r
}
define i16 @ashr10(i16 %x) { ; O2-LABEL: ashr10:
; O2: asr16i r4, 10
  %r = ashr i16 %x, 10
  ret i16 %r
}
define i16 @ashr11(i16 %x) { ; O2-LABEL: ashr11:
; O2: asr16i r4, 11
  %r = ashr i16 %x, 11
  ret i16 %r
}
define i16 @ashr12(i16 %x) { ; O2-LABEL: ashr12:
; O2: asr16i r4, 12
  %r = ashr i16 %x, 12
  ret i16 %r
}
define i16 @ashr13(i16 %x) { ; O2-LABEL: ashr13:
; O2: asr16i r4, 13
  %r = ashr i16 %x, 13
  ret i16 %r
}
define i16 @ashr14(i16 %x) { ; O2-LABEL: ashr14:
; O2: asr16i r4, 14
  %r = ashr i16 %x, 14
  ret i16 %r
}
define i16 @ashr15(i16 %x) { ; O2-LABEL: ashr15:
; O2: asr16i r4, 15
  %r = ashr i16 %x, 15
  ret i16 %r
}

define i16 @shl_out_of_range(i16 %x) {
; O2-LABEL: shl_out_of_range:
; O2-NOT:   shl16v
; O2:       ret
  %r = shl i16 %x, 16
  ret i16 %r
}

define i16 @lshr_out_of_range(i16 %x) {
; O2-LABEL: lshr_out_of_range:
; O2-NOT:   lsr16v
; O2:       ret
  %r = lshr i16 %x, 16
  ret i16 %r
}

define i16 @ashr_out_of_range(i16 %x) {
; O2-LABEL: ashr_out_of_range:
; O2-NOT:   asr16v
; O2:       ret
  %r = ashr i16 %x, 16
  ret i16 %r
}

define i16 @shl_variable(i16 %x, i16 %count) {
; O2-LABEL: shl_variable:
; O2:       shl16v r4, r5
  %r = shl i16 %x, %count
  ret i16 %r
}

define i16 @lshr_variable(i16 %x, i16 %count) {
; O2-LABEL: lshr_variable:
; O2:       lsr16v r4, r5
  %r = lshr i16 %x, %count
  ret i16 %r
}

define i16 @ashr_variable(i16 %x, i16 %count) {
; O2-LABEL: ashr_variable:
; O2:       asr16v r4, r5
  %r = ashr i16 %x, %count
  ret i16 %r
}

define i16 @size_shl1(i16 %x) #0 {
; SIZE-LABEL: size_shl1:
; SIZE:       lsl16i r4, 1
  %r = shl i16 %x, 1
  ret i16 %r
}

define i16 @size_lshr1(i16 %x) #0 {
; SIZE-LABEL: size_lshr1:
; SIZE:       lsr16i r4, 1
  %r = lshr i16 %x, 1
  ret i16 %r
}

define i16 @minsize_ashr1(i16 %x) #1 {
; SIZE-LABEL: minsize_ashr1:
; SIZE:       asr16i r4, 1
  %r = ashr i16 %x, 1
  ret i16 %r
}

attributes #0 = { optsize }
attributes #1 = { minsize optsize }
