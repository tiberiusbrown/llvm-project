; RUN: opt -mtriple=avm-unknown-arduboyfx -mcpu=avm1 \
; RUN:   -mtune=avm-interpreter-32u4-v1 -passes=loop-idiom -S %s | FileCheck %s

; AVM has no measured lookup load for a 64-bit table entry, so the table query reports
; no cost and the bit-serial loop is kept. The generic target still forms the table for
; the same loop, so the rejection comes from the AVM cost, not from recognition.

; CHECK-NOT: crctable
; CHECK-LABEL: define i64 @crc64.le.tc8(
; CHECK:       %crc = phi i64
; CHECK:       %crc.lshr = lshr i64 %crc, 1

define i64 @crc64.le.tc8(i8 %msg, i64 %checksum) {
entry:
  br label %loop

loop:
  %iv = phi i8 [ 0, %entry ], [ %iv.next, %loop ]
  %crc = phi i64 [ %checksum, %entry ], [ %crc.next, %loop ]
  %data = phi i8 [ %msg, %entry ], [ %data.next, %loop ]
  %crc.trunc = trunc i64 %crc to i8
  %xor.data.crc = xor i8 %data, %crc.trunc
  %and.data.crc = and i8 %xor.data.crc, 1
  %data.next = lshr i8 %data, 1
  %check.sb = icmp eq i8 %and.data.crc, 0
  %crc.lshr = lshr i64 %crc, 1
  %xor = xor i64 %crc.lshr, -3932672073
  %crc.next = select i1 %check.sb, i64 %crc.lshr, i64 %xor
  %iv.next = add nuw nsw i8 %iv, 1
  %exit.cond = icmp samesign ult i8 %iv, 7
  br i1 %exit.cond, label %loop, label %exit

exit:
  ret i64 %crc.next
}
