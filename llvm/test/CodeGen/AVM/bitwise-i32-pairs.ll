; RUN: llc -mtriple=avm -O2 -stop-after=finalize-isel -verify-machineinstrs %s -o - | FileCheck %s
; RUN: llc -mtriple=avm -O2 -verify-machineinstrs -filetype=obj %s -o %t.o

; Full-width operations should retain their native pair constraints. Splitting
; these into independent words increases register pressure in wide arithmetic.
define i32 @full_or(i32 %a, i32 %b) {
; CHECK-LABEL: name: full_or
; CHECK-NOT: OR16_PSEUDO
; CHECK: OR32_PSEUDO
; CHECK-NEXT: $r4r5 = COPY
  %r = or i32 %a, %b
  ret i32 %r
}

define i32 @full_xor(i32 %a, i32 %b) {
; CHECK-LABEL: name: full_xor
; CHECK-NOT: XOR16_PSEUDO
; CHECK: XOR32_PSEUDO
; CHECK-NEXT: $r4r5 = COPY
  %r = xor i32 %a, %b
  ret i32 %r
}

define i32 @xor_low_byte(i32 %a, i8 zeroext %b) {
; CHECK-LABEL: name: xor_low_byte
; CHECK-NOT: XOR16_PSEUDO
; CHECK: XOR32_PSEUDO
; CHECK-NEXT: $r4r5 = COPY
  %bw = zext i8 %b to i32
  %r = xor i32 %a, %bw
  ret i32 %r
}
