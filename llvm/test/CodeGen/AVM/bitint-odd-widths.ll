; RUN: llc -mtriple=avm-unknown-arduboyfx -mcpu=avm1 -O0 -verify-machineinstrs < %s | FileCheck %s --check-prefixes=CHECK
; RUN: llc -mtriple=avm-unknown-arduboyfx -mcpu=avm1 -O2 -verify-machineinstrs < %s | FileCheck %s --check-prefixes=CHECK
; RUN: llc -mtriple=avm-unknown-arduboyfx -mcpu=avm1 -O2 -verify-machineinstrs -filetype=obj %s -o %t.o

; Integers of the C23 _BitInt widths that have no native AVM register form.
; Shifts and bit counts on these widths are computed in the next container:
; a 16-bit variable shift for i9, and the 64-bit helper for i40. The object
; run checks that every selected form also encodes.

declare i9 @llvm.ctlz.i9(i9, i1)
declare i40 @llvm.cttz.i40(i40, i1)
declare i12 @llvm.ctpop.i12(i12)
declare i48 @llvm.ctlz.i48(i48, i1)

; CHECK-LABEL: shl9:
; CHECK: shl16v
define i9 @shl9(i9 %a, i9 %b) {
  %r = shl i9 %a, %b
  ret i9 %r
}

; CHECK-LABEL: lshr12:
; CHECK: lsr16v
define i12 @lshr12(i12 %a, i12 %b) {
  %r = lshr i12 %a, %b
  ret i12 %r
}

; CHECK-LABEL: ashr40:
; CHECK: __avm_ashrdi3
define i40 @ashr40(i40 %a, i40 %b) {
  %r = ashr i40 %a, %b
  ret i40 %r
}

; CHECK-LABEL: shl40:
; CHECK: __avm_ashldi3
define i40 @shl40(i40 %a, i40 %b) {
  %r = shl i40 %a, %b
  ret i40 %r
}

; CHECK-LABEL: ctlz9:
define i9 @ctlz9(i9 %a) {
  %r = call i9 @llvm.ctlz.i9(i9 %a, i1 false)
  ret i9 %r
}

; CHECK-LABEL: cttz40:
define i40 @cttz40(i40 %a) {
  %r = call i40 @llvm.cttz.i40(i40 %a, i1 true)
  ret i40 %r
}

; CHECK-LABEL: ctpop12:
define i12 @ctpop12(i12 %a) {
  %r = call i12 @llvm.ctpop.i12(i12 %a)
  ret i12 %r
}

; CHECK-LABEL: ctlz48:
define i48 @ctlz48(i48 %a) {
  %r = call i48 @llvm.ctlz.i48(i48 %a, i1 false)
  ret i48 %r
}

declare {i24, i1} @llvm.sadd.with.overflow.i24(i24, i24)
declare {i40, i1} @llvm.umul.with.overflow.i40(i40, i40)

; The DAG combiner drops the sign extension of a 24-bit value loaded from
; memory when it uses sext. The widened compare extends with xor and subtract,
; so the signed result follows the loaded bytes.
; CHECK-LABEL: slt24:
; CHECK: xor
define i1 @slt24(ptr %p, ptr %q) {
  %a = load i24, ptr %p
  %b = load i24, ptr %q
  %c = icmp slt i24 %a, %b
  ret i1 %c
}

; CHECK-LABEL: sadd24:
define {i24, i1} @sadd24(i24 %a, i24 %b) {
  %r = call {i24, i1} @llvm.sadd.with.overflow.i24(i24 %a, i24 %b)
  ret {i24, i1} %r
}

; CHECK-LABEL: umul40:
define {i40, i1} @umul40(i40 %a, i40 %b) {
  %r = call {i40, i1} @llvm.umul.with.overflow.i40(i40 %a, i40 %b)
  ret {i40, i1} %r
}

; Signed division of an odd width runs as a 32-bit division.
; CHECK-LABEL: sdiv24:
; CHECK: __avm_divsi3
define i24 @sdiv24(i24 %a, i24 %b) {
  %r = sdiv i24 %a, %b
  ret i24 %r
}

; CHECK-LABEL: sitofp24:
define float @sitofp24(i24 %a) {
  %r = sitofp i24 %a to float
  ret float %r
}

; CHECK-LABEL: fptosi24:
define i24 @fptosi24(float %a) {
  %r = fptosi float %a to i24
  ret i24 %r
}
