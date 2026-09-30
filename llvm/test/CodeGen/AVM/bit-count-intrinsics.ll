; RUN: llc -mtriple=avm-unknown-arduboyfx -mcpu=avm1 \
; RUN:   -mtune=avm-interpreter-32u4-v1 -O2 -verify-machineinstrs < %s \
; RUN:   | FileCheck %s

declare i16 @llvm.ctpop.i16(i16)
declare i16 @llvm.ctlz.i16(i16, i1)
declare i16 @llvm.cttz.i16(i16, i1)

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
