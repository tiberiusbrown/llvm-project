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

define i16 @reconstructed_unsigned_rem(i16 %x, i16 %y) #0 {
; CHECK-LABEL: reconstructed_unsigned_rem:
; CHECK: urem16
; CHECK-NOT: udiv16
  %q = udiv i16 %x, %y
  %p = mul i16 %q, %y
  %r = sub i16 %x, %p
  ret i16 %r
}

define i16 @reconstructed_signed_rem(i16 %x, i16 %y) #0 {
; CHECK-LABEL: reconstructed_signed_rem:
; CHECK: srem16
; CHECK-NOT: sdiv16
  %q = sdiv i16 %x, %y
  %p = mul i16 %y, %q
  %r = sub i16 %x, %p
  ret i16 %r
}

define i32 @shared_unsigned_quotient(i16 %x, i16 %y) #0 {
; CHECK-LABEL: shared_unsigned_quotient:
; CHECK: udiv16
; CHECK: mul16
; CHECK: sub
; CHECK-NOT: urem16
  %q = udiv i16 %x, %y
  %p = mul i16 %q, %y
  %r = sub i16 %x, %p
  %q.wide = zext i16 %q to i32
  %r.wide = zext i16 %r to i32
  %r.high = shl i32 %r.wide, 16
  %both = or i32 %q.wide, %r.high
  ret i32 %both
}

define i32 @shared_signed_quotient(i16 %x, i16 %y) #0 {
; CHECK-LABEL: shared_signed_quotient:
; CHECK: sdiv16
; CHECK: mul16
; CHECK: sub
; CHECK-NOT: srem16
  %q = sdiv i16 %x, %y
  %p = mul i16 %q, %y
  %r = sub i16 %x, %p
  %q.wide = zext i16 %q to i32
  %r.wide = zext i16 %r to i32
  %r.high = shl i32 %r.wide, 16
  %both = or i32 %q.wide, %r.high
  ret i32 %both
}

attributes #0 = { optsize }
