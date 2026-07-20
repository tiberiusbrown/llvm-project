; RUN: llc -mtriple=avm-unknown-arduboyfx -O0 \
; RUN:   -verify-machineinstrs < %s | FileCheck %s --check-prefix=CHECK
; RUN: llc -mtriple=avm-unknown-arduboyfx -O2 \
; RUN:   -verify-machineinstrs < %s | FileCheck %s --check-prefix=CHECK

define float @return_float_one() {
; CHECK-LABEL: return_float_one:
; CHECK: ldi8
; CHECK: ldi16
  ret float 1.000000e+00
}

define void @store_float_one(ptr %dst) {
; CHECK-LABEL: store_float_one:
; CHECK: ldi8
; CHECK: ldi16
  store volatile float 1.000000e+00, ptr %dst, align 1
  ret void
}

define i1 @compare_float_one(float %value) {
; CHECK-LABEL: compare_float_one:
; CHECK: ldi8
; CHECK: ldi16
; CHECK: fcmp
  %equal = fcmp oeq float %value, 1.000000e+00
  ret i1 %equal
}

define float @return_negative_float() {
; CHECK-LABEL: return_negative_float:
; CHECK: ldi8
; CHECK: ldi16
  ret float -2.500000e+00
}
