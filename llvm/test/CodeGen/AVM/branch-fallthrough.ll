; RUN: llc -mtriple=avm -O2 -verify-machineinstrs < %s | FileCheck %s

define void @countdown(ptr %out) {
; CHECK-LABEL: countdown:
entry:
  br label %loop

loop:
; CHECK:       LBB0_1:
  %value = phi i16 [ 4, %entry ], [ %next, %loop ]
  store volatile i16 %value, ptr %out, align 1
  %next = add i16 %value, -1
  %done = icmp eq i16 %next, 0
  br i1 %done, label %exit, label %loop
; CHECK:       brne{{(8|16)?}} LBB0_1
; CHECK-NOT:   jmp{{8|16}}

exit:
  ret void
}
