; RUN: llc -mtriple=avm-unknown-arduboyfx -mcpu=avm1 \
; RUN:   -mtune=avm-interpreter-32u4-v1 -O2 -verify-machineinstrs < %s \
; RUN:   | FileCheck %s

declare i16 @llvm.ctpop.i16(i16)
declare i16 @llvm.ctlz.i16(i16, i1)
declare i16 @llvm.cttz.i16(i16, i1)
declare i32 @llvm.ctpop.i32(i32)
declare i32 @llvm.ctlz.i32(i32, i1)

define i16 @popcount(i16 %x) {
; CHECK-LABEL: popcount:
; CHECK:       and
; CHECK:       ret
  %count = call i16 @llvm.ctpop.i16(i16 %x)
  ret i16 %count
}

define i16 @leading_zeros(i16 %x) {
; CHECK-LABEL: leading_zeros:
; CHECK:       lsr16
; CHECK:       ret
  %count = call i16 @llvm.ctlz.i16(i16 %x, i1 true)
  ret i16 %count
}

define i16 @trailing_zeros(i16 %x) {
; CHECK-LABEL: trailing_zeros:
; CHECK:       and
; CHECK:       ret
  %count = call i16 @llvm.cttz.i16(i16 %x, i1 true)
  ret i16 %count
}

define i16 @leading_zeros_defined(i16 %x) {
; CHECK-LABEL: leading_zeros_defined:
; CHECK:       ret
  %count = call i16 @llvm.ctlz.i16(i16 %x, i1 false)
  ret i16 %count
}

define i16 @trailing_zeros_defined(i16 %x) {
; CHECK-LABEL: trailing_zeros_defined:
; CHECK:       ret
  %count = call i16 @llvm.cttz.i16(i16 %x, i1 false)
  ret i16 %count
}

define i32 @popcount32(i32 %x) {
; CHECK-LABEL: popcount32:
; CHECK-NOT:   call
; CHECK:       and
; CHECK:       ret
  %count = call i32 @llvm.ctpop.i32(i32 %x)
  ret i32 %count
}

define i32 @leading_zeros32(i32 %x) {
; CHECK-LABEL: leading_zeros32:
; CHECK-NOT:   call
; CHECK:       lsr32
; CHECK:       ret
  %count = call i32 @llvm.ctlz.i32(i32 %x, i1 true)
  ret i32 %count
}

define i32 @leading_zeros32_defined(i32 %x) {
; CHECK-LABEL: leading_zeros32_defined:
; CHECK:       ret
  %count = call i32 @llvm.ctlz.i32(i32 %x, i1 false)
  ret i32 %count
}
