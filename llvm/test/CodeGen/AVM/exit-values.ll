; RUN: llc -mtriple=avm -O2 -verify-machineinstrs < %s | FileCheck %s

; The null return constant must not occupy r4 throughout the scan. Its pointer
; can remain in the ABI argument/return register and use the compact RAM load.
define ptr @find_byte(ptr %base, i8 %needle, i16 %count) {
; CHECK-LABEL: find_byte:
; CHECK-NOT: push16
; CHECK: ld8u {{r[4-7]}}, [r4]
; CHECK-NOT: pop16
; CHECK: ret
entry:
  %empty = icmp eq i16 %count, 0
  br i1 %empty, label %notfound, label %loop
loop:
  %p = phi ptr [ %base, %entry ], [ %next, %latch ]
  %n = phi i16 [ %count, %entry ], [ %nn, %latch ]
  %value = load i8, ptr %p, align 1
  %found = icmp eq i8 %value, %needle
  br i1 %found, label %success, label %latch
latch:
  %next = getelementptr inbounds i8, ptr %p, i16 1
  %nn = add i16 %n, -1
  %done = icmp eq i16 %nn, 0
  br i1 %done, label %notfound, label %loop
success:
  ret ptr %p
notfound:
  ret ptr null
}
