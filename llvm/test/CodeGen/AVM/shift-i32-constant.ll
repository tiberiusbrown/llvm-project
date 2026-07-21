; RUN: llc -mtriple=avm -O2 -verify-machineinstrs < %s | FileCheck %s
; RUN: llc -mtriple=avm -O2 -verify-machineinstrs < %s | FileCheck %s --check-prefix=NOHELP
; RUN: llc -mtriple=avm -O2 -verify-machineinstrs -filetype=obj %s -o %t.o

; NOHELP-NOT: __avm_ashlsi3
; NOHELP-NOT: __avm_lshrsi3
; NOHELP-NOT: __avm_ashrsi3

define i32 @shl1(i32 %x) {
; CHECK-LABEL: shl1:
; CHECK: add32
  %r = shl i32 %x, 1
  ret i32 %r
}

define i32 @lshr1(i32 %x) {
; CHECK-LABEL: lshr1:
; CHECK: lsr32.1
  %r = lshr i32 %x, 1
  ret i32 %r
}

define i32 @ashr1(i32 %x) {
; CHECK-LABEL: ashr1:
; CHECK: asr32.1
  %r = ashr i32 %x, 1
  ret i32 %r
}

define i32 @shl8(i32 %x) {
  %r = shl i32 %x, 8
  ret i32 %r
}
define i32 @shl15(i32 %x) {
  %r = shl i32 %x, 15
  ret i32 %r
}
define i32 @shl16(i32 %x) {
  %r = shl i32 %x, 16
  ret i32 %r
}
define i32 @shl17(i32 %x) {
  %r = shl i32 %x, 17
  ret i32 %r
}
define i32 @shl24(i32 %x) {
  %r = shl i32 %x, 24
  ret i32 %r
}
define i32 @shl31(i32 %x) {
  %r = shl i32 %x, 31
  ret i32 %r
}

define i32 @lshr8(i32 %x) {
  %r = lshr i32 %x, 8
  ret i32 %r
}
define i32 @lshr15(i32 %x) {
  %r = lshr i32 %x, 15
  ret i32 %r
}
define i32 @lshr16(i32 %x) {
  %r = lshr i32 %x, 16
  ret i32 %r
}
define i32 @lshr17(i32 %x) {
  %r = lshr i32 %x, 17
  ret i32 %r
}
define i32 @lshr24(i32 %x) {
  %r = lshr i32 %x, 24
  ret i32 %r
}
define i32 @lshr31(i32 %x) {
  %r = lshr i32 %x, 31
  ret i32 %r
}

define i32 @ashr8(i32 %x) {
  %r = ashr i32 %x, 8
  ret i32 %r
}
define i32 @ashr15(i32 %x) {
  %r = ashr i32 %x, 15
  ret i32 %r
}
define i32 @ashr16(i32 %x) {
  %r = ashr i32 %x, 16
  ret i32 %r
}
define i32 @ashr17(i32 %x) {
  %r = ashr i32 %x, 17
  ret i32 %r
}
define i32 @ashr24(i32 %x) {
  %r = ashr i32 %x, 24
  ret i32 %r
}
define i32 @ashr31(i32 %x) {
  %r = ashr i32 %x, 31
  ret i32 %r
}
