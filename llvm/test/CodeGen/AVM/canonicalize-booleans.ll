; RUN: llc -mtriple=avm -O2 -verify-machineinstrs \
; RUN:   -stop-after=avm-canonicalize-booleans -o - %s | FileCheck %s

target triple = "avm"

declare void @true_path() addrspace(1)
declare void @false_path() addrspace(1)

; A general i1 PHI is promoted with undefined upper bits.  This recurrence is
; nevertheless provably zero-or-one, so the AND 1 inserted for BRCOND is dead.
;
; CHECK-LABEL: name: canonical_phi
; CHECK: PHI
; CHECK-NOT: AND16_PSEUDO
; CHECK: TST8_PSEUDO
; CHECK: XOR16_PSEUDO

define void @canonical_phi() addrspace(1) {
entry:
  br label %loop

loop:
  %value = phi i1 [ false, %entry ], [ %next, %latch ]
  br i1 %value, label %true, label %false

true:
  call addrspace(1) void @true_path()
  br label %latch

false:
  call addrspace(1) void @false_path()
  br label %latch

latch:
  %next = xor i1 %value, true
  br label %loop
}

; An arbitrary value truncated to i1 is not a canonical full-register boolean.
; Its AND 1 must remain: testing the original i16 would be wrong for values such
; as two.
;
; CHECK-LABEL: name: truncated_i1
; CHECK: AND16_PSEUDO
; CHECK: TST8_PSEUDO

define void @truncated_i1(i16 %value) addrspace(1) {
entry:
  %bit = trunc i16 %value to i1
  br i1 %bit, label %true, label %false

true:
  call addrspace(1) void @true_path()
  ret void

false:
  call addrspace(1) void @false_path()
  ret void
}
