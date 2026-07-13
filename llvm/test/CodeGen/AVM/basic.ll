; RUN: llc -mtriple=avm-unknown-arduboyfx < %s | FileCheck %s

define i16 @add_and_test(i16 %a, i16 %b) {
entry:
  %result = add i16 %a, %b
  %test = icmp eq i16 %result, 7
  br i1 %test, label %yes, label %no

yes:
  ret i16 %result

no:
  ret i16 0
}

; CHECK-LABEL: add_and_test:
; CHECK: add
; CHECK: cmp16
; CHECK: cset
; CHECK: brne
; CHECK: ret

define i16 @logical_and_subtract(i16 %a, i16 %b, i16 %c) {
entry:
  %and = and i16 %a, %b
  %or = or i16 %and, %c
  %xor = xor i16 %or, %b
  %result = sub i16 %xor, %a
  ret i16 %result
}

; CHECK-LABEL: logical_and_subtract:
; CHECK: and
; CHECK: or
; CHECK: xor
; CHECK: sub
; CHECK: ret

define i16 @load_and_store(ptr %address, i16 %value) {
entry:
  store i16 %value, ptr %address, align 1
  %result = load i16, ptr %address, align 1
  ret i16 %result
}

; CHECK-LABEL: load_and_store:
; CHECK: st16
; CHECK: ld16
; CHECK: ret

define ptr @load_pointer(ptr %slot) {
entry:
  %result = load ptr, ptr %slot, align 1
  ret ptr %result
}

; CHECK-LABEL: load_pointer:
; CHECK: ld16
; CHECK: ret

define zeroext i8 @return_unsigned_byte(i8 zeroext %value) {
entry:
  ret i8 %value
}

; CHECK-LABEL: return_unsigned_byte:
; CHECK: mov8z
; CHECK: ret

define signext i8 @return_signed_byte(i8 signext %value) {
entry:
  ret i8 %value
}

; CHECK-LABEL: return_signed_byte:
; CHECK: mov8s
; CHECK: ret

define zeroext i8 @byte_add_and_xor(i8 zeroext %a, i8 zeroext %b) {
entry:
  %sum = add i8 %a, %b
  %result = xor i8 %sum, %b
  ret i8 %result
}

; CHECK-LABEL: byte_add_and_xor:
; CHECK: add
; CHECK: xor
; CHECK: mov8z
; CHECK: ret

define zeroext i1 @return_bool(i1 zeroext %value) {
entry:
  ret i1 %value
}

; CHECK-LABEL: return_bool:
; CHECK: mov8z
; CHECK: ret

define zeroext i8 @load_and_store_byte(ptr %address, i8 zeroext %value) {
entry:
  store i8 %value, ptr %address, align 1
  %result = load i8, ptr %address, align 1
  ret i8 %result
}

; CHECK-LABEL: load_and_store_byte:
; CHECK: mov8z
; CHECK: st8
; CHECK: ld8
; CHECK: ret
