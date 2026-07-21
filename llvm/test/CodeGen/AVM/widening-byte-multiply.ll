; RUN: llc -mtriple=avm -O2 -verify-machineinstrs < %s | FileCheck %s

define i16 @multiply_known_unsigned_bytes(i8 zeroext %a, i8 zeroext %b) {
; CHECK-LABEL: multiply_known_unsigned_bytes:
; CHECK:       mulu8.w
; CHECK-NOT:   mul16
entry:
  %a16 = zext i8 %a to i16
  %b16 = zext i8 %b to i16
  %combined = or i16 %a16, %b16
  %result = mul i16 %combined, %combined
  ret i16 %result
}

define i16 @multiply_promoted_narrow_induction(i16 %start) {
; CHECK-LABEL: multiply_promoted_narrow_induction:
; CHECK:       mulu8.w
; CHECK-NOT:   mul16
entry:
  br label %loop

loop:
  %value = phi i16 [ %start, %entry ], [ %next, %loop ]
  %result = mul nuw i16 %value, %value
  %next = add nuw i16 %value, 1
  %done = icmp eq i16 %next, 30
  br i1 %done, label %exit, label %loop

exit:
  ret i16 %result
}
