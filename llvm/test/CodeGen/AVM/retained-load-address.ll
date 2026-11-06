; RUN: llc -mtriple=avm -O2 -verify-machineinstrs < %s | FileCheck %s

; Retaining the loaded address in a conditional result should leave the
; increment after the select, avoiding two copies around a tied indexed load.
define ptr @last_match(ptr %base, i8 %needle) {
; CHECK-LABEL: last_match:
; CHECK: ld8u {{r[0-7]}}, [{{r[0-7]}}]
; CHECK-NOT: ld8u {{.*}}+]
; CHECK: cmov.eq
; CHECK: inc16
entry:
  br label %loop
loop:
  %p = phi ptr [ %base, %entry ], [ %next, %loop ]
  %last = phi ptr [ null, %entry ], [ %selected, %loop ]
  %v = load i8, ptr %p, align 1
  %match = icmp eq i8 %v, %needle
  %selected = select i1 %match, ptr %p, ptr %last
  %next = getelementptr i8, ptr %p, i16 1
  %more = icmp ne i8 %v, 0
  br i1 %more, label %loop, label %done
done:
  ret ptr %selected
}
