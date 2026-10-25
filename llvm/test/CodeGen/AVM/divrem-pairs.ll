; RUN: opt -mtriple=avm -passes=div-rem-pairs < %s | llc -mtriple=avm -O2 -verify-machineinstrs | FileCheck %s --implicit-check-not=__avm_mul
; RUN: opt -mtriple=avm -passes=div-rem-pairs -S < %s | FileCheck %s --check-prefix=IR
; RUN: llc -mtriple=avm -O0 -verify-machineinstrs < %s | FileCheck %s --check-prefix=SINGLE

define i32 @unsigned32(i32 %n, i32 %d, ptr %remainder) {
; CHECK-LABEL: unsigned32:
; CHECK-NOT:   call
; CHECK:       call __avm_udivmodsi4
; CHECK-NOT:   call
; CHECK:       ret
  %q = udiv i32 %n, %d
  %r = urem i32 %n, %d
  store i32 %r, ptr %remainder, align 1
  ret i32 %q
}

define i32 @signed32(i32 %n, i32 %d, ptr %remainder) {
; CHECK-LABEL: signed32:
; CHECK-NOT:   call
; CHECK:       call __avm_divmodsi4
; CHECK-NOT:   call
; CHECK:       ret
  %q = sdiv i32 %n, %d
  %r = srem i32 %n, %d
  store i32 %r, ptr %remainder, align 1
  ret i32 %q
}

define i64 @unsigned64(i64 %n, i64 %d, ptr %remainder) {
; CHECK-LABEL: unsigned64:
; CHECK-NOT:   call
; CHECK:       call __avm_udivmoddi4
; CHECK-NOT:   call
; CHECK:       ret
  %q = udiv i64 %n, %d
  %r = urem i64 %n, %d
  store i64 %r, ptr %remainder, align 1
  ret i64 %q
}

define i64 @signed64(i64 %n, i64 %d, ptr %remainder) {
; CHECK-LABEL: signed64:
; CHECK-NOT:   call
; CHECK:       call __avm_divmoddi4
; CHECK-NOT:   call
; CHECK:       ret
  %q = sdiv i64 %n, %d
  %r = srem i64 %n, %d
  store i64 %r, ptr %remainder, align 1
  ret i64 %q
}

; DivRemPairs must preserve/recompose the pair before SelectionDAG, including
; when the source already spells out the remainder using multiply/subtract.
define i64 @expanded64(i64 %n, i64 %d, ptr %remainder) {
; CHECK-LABEL: expanded64:
; CHECK-NOT:   call
; CHECK:       call __avm_udivmoddi4
; CHECK-NOT:   call
; CHECK:       ret
  %q = udiv i64 %n, %d
  %product = mul i64 %q, %d
  %r = sub i64 %n, %product
  store i64 %r, ptr %remainder, align 1
  ret i64 %q
}

; TTI must also allow DivRemPairs to hoist a matching remainder in IR.
define i32 @conditional32(i32 %n, i32 %d, ptr %remainder, i1 %store) {
; IR-LABEL: define i32 @conditional32(
; IR:         %q = udiv i32 %n, %d
; IR-NEXT:    %r = urem i32 %n, %d
; IR-NEXT:    br i1 %store
  %q = udiv i32 %n, %d
  br i1 %store, label %write, label %exit
write:
  %r = urem i32 %n, %d
  store i32 %r, ptr %remainder, align 1
  br label %exit
exit:
  ret i32 %q
}

; Native i16 division keeps quotient reuse with the cheap native product.
define i16 @unsigned16(i16 %n, i16 %d, ptr %remainder) {
; CHECK-LABEL: unsigned16:
; CHECK-NOT:   call
; CHECK:       udiv16
; CHECK-NOT:   urem16
; CHECK:       mul16
; CHECK:       sub
; CHECK:       ret
  %q = udiv i16 %n, %d
  %r = urem i16 %n, %d
  store i16 %r, ptr %remainder, align 1
  ret i16 %q
}

; Standalone operations keep their original helper ABI at both opt levels.
define i64 @quotient64(i64 %n, i64 %d) {
; CHECK-LABEL: quotient64:
; CHECK:       call __avm_udivdi3
; CHECK-NOT:   call
; CHECK:       ret
; SINGLE-LABEL: quotient64:
; SINGLE:       call __avm_udivdi3
  %q = udiv i64 %n, %d
  ret i64 %q
}

define i64 @remainder64(i64 %n, i64 %d) {
; CHECK-LABEL: remainder64:
; CHECK:       call __avm_umoddi3
; CHECK-NOT:   call
; CHECK:       ret
; SINGLE-LABEL: remainder64:
; SINGLE:       call __avm_umoddi3
  %r = urem i64 %n, %d
  ret i64 %r
}

define i64 @signed_quotient64(i64 %n, i64 %d) {
; CHECK-LABEL: signed_quotient64:
; CHECK:       call __avm_divdi3
; CHECK-NOT:   call
; CHECK:       ret
; SINGLE-LABEL: signed_quotient64:
; SINGLE:       call __avm_divdi3
  %q = sdiv i64 %n, %d
  ret i64 %q
}

define i64 @signed_remainder64(i64 %n, i64 %d) {
; CHECK-LABEL: signed_remainder64:
; CHECK:       call __avm_moddi3
; CHECK-NOT:   call
; CHECK:       ret
; SINGLE-LABEL: signed_remainder64:
; SINGLE:       call __avm_moddi3
  %r = srem i64 %n, %d
  ret i64 %r
}

; Mismatched operands and signedness must not be combined.
define i64 @different_divisor(i64 %n, i64 %d, i64 %other, ptr %out) {
; CHECK-LABEL: different_divisor:
; CHECK-NOT:   divmod
; CHECK-DAG:   call __avm_udivdi3
; CHECK-DAG:   call __avm_umoddi3
; CHECK-NOT:   divmod
; CHECK:       ret
  %q = udiv i64 %n, %d
  %r = urem i64 %n, %other
  store i64 %r, ptr %out, align 1
  ret i64 %q
}

define i32 @different_sign(i32 %n, i32 %d, ptr %out) {
; CHECK-LABEL: different_sign:
; CHECK-NOT:   divmod
; CHECK-DAG:   call __avm_udivsi3
; CHECK-DAG:   call __avm_modsi3
; CHECK-NOT:   divmod
; CHECK:       ret
  %q = udiv i32 %n, %d
  %r = srem i32 %n, %d
  store i32 %r, ptr %out, align 1
  ret i32 %q
}
