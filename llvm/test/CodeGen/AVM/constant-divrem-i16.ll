; RUN: llc -mtriple=avm-unknown-arduboyfx -O2 \
; RUN:   -verify-machineinstrs < %s | FileCheck %s

define i16 @signed_div3(i16 %value) #0 {
; CHECK-LABEL: signed_div3:
; CHECK: sdiv16
; CHECK-NOT: __avm_mulsi3
  %result = sdiv i16 %value, 3
  ret i16 %result
}

define i16 @signed_rem5(i16 %value) #0 {
; CHECK-LABEL: signed_rem5:
; CHECK: srem16
; CHECK-NOT: __avm_mulsi3
  %result = srem i16 %value, 5
  ret i16 %result
}

define i16 @unsigned_div7(i16 %value) #0 {
; CHECK-LABEL: unsigned_div7:
; CHECK: udiv16
; CHECK-NOT: __avm_mulsi3
  %result = udiv i16 %value, 7
  ret i16 %result
}

define i16 @unsigned_rem7(i16 %value) #0 {
; CHECK-LABEL: unsigned_rem7:
; CHECK: urem16
; CHECK-NOT: __avm_mulsi3
  %result = urem i16 %value, 7
  ret i16 %result
}

attributes #0 = { optsize }
