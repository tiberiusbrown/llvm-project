; RUN: llc -mtriple=avm -O2 -disable-lsr -stop-after=avm-byte-phis %s -o - | FileCheck %s --check-prefix=IR
; RUN: llc -mtriple=avm -O2 -verify-machineinstrs %s -o - | FileCheck %s

; Widen the recurrence and its latch comparison together. No normalization is
; needed even when the byte also has memory, signed comparison and ABI users.
declare void @take_byte(i8 signext)

define void @bounded_increment(ptr %out) {
; IR-LABEL: define void @bounded_increment(
; IR: %i.word = phi i16 [ 0, %entry ], [ %next.word, %loop ]
; IR: store volatile i8
; IR: %next.word = add nuw i16 %i.word, 1
; IR: icmp eq i16 %next.word, 32
; CHECK-LABEL: bounded_increment:
; CHECK-NOT: zext8
; CHECK: inc16 [[COUNTER:r[0-9]+]]
; CHECK-NEXT: cmpi.s8 [[COUNTER]], 32
; CHECK-NOT: zext8
; CHECK: ret
entry:
  br label %loop
loop:
  %i = phi i8 [0, %entry], [%next, %loop]
  store volatile i8 %i, ptr %out
  %next = add nuw nsw i8 %i, 1
  %done = icmp eq i8 %next, 32
  br i1 %done, label %exit, label %loop
exit:
  ret void
}

; The same proof may come from the loop range rather than an IR flag.
define i16 @inferred_increment(ptr %out) {
; IR-LABEL: define i16 @inferred_increment(
; IR: %i.word = phi i16
; IR: %next.word = add nuw i16 %i.word, 1
; IR: icmp eq i16 %next.word, 200
; IR: ret i16 %next.word
; CHECK-LABEL: inferred_increment:
; CHECK-NOT: zext8
; CHECK: inc16
; CHECK-NOT: zext8
; CHECK: ret
entry:
  br label %loop
loop:
  %i = phi i8 [0, %entry], [%next, %loop]
  store volatile i8 %i, ptr %out
  %next = add i8 %i, 1
  %done = icmp eq i8 %next, 200
  br i1 %done, label %exit, label %loop
exit:
  %result = zext i8 %next to i16
  ret i16 %result
}

; Keep both the variable subtraction and its unsigned comparison in words.
define i16 @variable_subtraction(ptr %p) {
; IR-LABEL: define i16 @variable_subtraction(
; IR: %i.word = phi i16
; IR: %step = load volatile i8
; IR: %step.word = zext i8 %step to i16
; IR: %next.word = sub nuw i16 %i.word, %step.word
; IR: icmp ult i16 %next.word, 100
; IR: ret i16 %next.word
; CHECK-LABEL: variable_subtraction:
; CHECK-NOT: zext8
; CHECK: sub
; CHECK-NOT: zext8
; CHECK: ret
entry:
  br label %loop
loop:
  %i = phi i8 [255, %entry], [%next, %loop]
  %step = load volatile i8, ptr %p
  %next = sub nuw i8 %i, %step
  %done = icmp ult i8 %next, 100
  br i1 %done, label %exit, label %loop
exit:
  %result = zext i8 %next to i16
  ret i16 %result
}

define i16 @bounded_decrement(ptr %out) {
; IR-LABEL: define i16 @bounded_decrement(
; IR: %i.word = phi i16 [ 255, %entry ], [ %next.word, %loop ]
; IR: %next.word = sub nuw i16 %i.word, 1
; IR: trunc i16 %next.word to i8
; IR: icmp eq i8 {{.*}}, 0
; CHECK-LABEL: bounded_decrement:
; CHECK-NOT: zext8
; CHECK: dec16
; CHECK-NOT: zext8
; CHECK: ret
entry:
  br label %loop
loop:
  %i = phi i8 [255, %entry], [%next, %loop]
  store volatile i8 %i, ptr %out
  %next = sub i8 %i, 1
  %done = icmp eq i8 %next, 0
  br i1 %done, label %exit, label %loop
exit:
  %result = zext i8 %next to i16
  ret i16 %result
}

; This recurrence has no consumer needing normalization. Keep the byte test
; against zero, which already lowers directly to TST8.
define void @byte_only_decrement(ptr %out) {
; IR-LABEL: define void @byte_only_decrement(
; IR-NOT: .word
; IR: %i = phi i8
; IR: store volatile i8
; IR: %next = add i8 %i, -1
; IR: icmp eq i8 %next, 0
; CHECK-LABEL: byte_only_decrement:
; CHECK-NOT: zext8
; CHECK: dec16
; CHECK-NEXT: tst8
; CHECK-NOT: zext8
; CHECK: ret
entry:
  br label %loop
loop:
  %i = phi i8 [32, %entry], [%next, %loop]
  store volatile i8 %i, ptr %out
  %next = sub i8 %i, 1
  %done = icmp eq i8 %next, 0
  br i1 %done, label %exit, label %loop
exit:
  ret void
}

; A coupled PHI graph can contain a chain of proven byte arithmetic. Preserve
; signed comparisons and byte ABI/memory uses with truncations.
define i32 @coupled_updates(ptr %out, ptr addrspace(1) %p) {
; IR-LABEL: define i32 @coupled_updates(
; IR: %i.word = phi i16
; IR: %a.word = phi i16
; IR: %b.word = phi i16
; IR: icmp slt i8
; IR: call {{.*}}void @take_byte(i8 signext
; IR: %step = load volatile i8, ptr addrspace(1) %p
; IR: %step.word = zext i8 %step to i16
; IR: %added.word = add nuw i16 %a.word, %step.word
; IR: %doubled.word = add nuw i16 %added.word, %step.word
; IR: %next.word = add nuw i16 %i.word, 1
; IR: icmp eq i16 %next.word, 32
; IR: zext i16 %b.word to i32
; CHECK-LABEL: coupled_updates:
; CHECK: ret
entry:
  br label %loop
loop:
  %i = phi i8 [0, %entry], [%next, %latch]
  %a = phi i8 [0, %entry], [%b, %latch]
  %b = phi i8 [128, %entry], [%doubled, %latch]
  %negative = icmp slt i8 %a, 0
  br i1 %negative, label %signed, label %latch
signed:
  call void @take_byte(i8 signext %a)
  br label %latch
latch:
  store volatile i8 %a, ptr %out
  %step = load volatile i8, ptr addrspace(1) %p
  %added = add nuw i8 %a, %step
  store volatile i8 %added, ptr %out
  %doubled = add nuw i8 %added, %step
  %next = add nuw nsw i8 %i, 1
  %done = icmp eq i8 %next, 32
  br i1 %done, label %exit, label %loop
exit:
  %result = zext i8 %b to i32
  ret i32 %result
}

; This increment crosses 255 before the exit. NSW is insufficient: -1 + 1
; does not overflow signed i8, but its unsigned result wraps from 255 to 0.
define i16 @unsigned_wrap(ptr %out) {
; IR-LABEL: define i16 @unsigned_wrap(
; IR-NOT: .word
; IR: %i = phi i8
; IR: %next = add nsw i8 %i, 1
; IR: icmp eq i8 %next, 2
; CHECK-LABEL: unsigned_wrap:
; CHECK: zext8
; CHECK: ret
entry:
  br label %loop
loop:
  %i = phi i8 [254, %entry], [%next, %loop]
  store volatile i8 %i, ptr %out
  %next = add nsw i8 %i, 1
  %done = icmp eq i8 %next, 2
  br i1 %done, label %exit, label %loop
exit:
  %result = zext i8 %next to i16
  ret i16 %result
}

define i16 @unsigned_underflow(ptr %out) {
; IR-LABEL: define i16 @unsigned_underflow(
; IR-NOT: .word
; IR: %i = phi i8
; IR: %next = add nsw i8 %i, -1
; IR: icmp eq i8 %next, -2
; CHECK-LABEL: unsigned_underflow:
; CHECK: zext8
; CHECK: ret
entry:
  br label %loop
loop:
  %i = phi i8 [1, %entry], [%next, %loop]
  store volatile i8 %i, ptr %out
  %next = sub nsw i8 %i, 1
  %done = icmp eq i8 %next, 254
  br i1 %done, label %exit, label %loop
exit:
  %result = zext i8 %next to i16
  ret i16 %result
}

; One wrapping input must invalidate the entire dependent graph, including a
; nonwrapping arithmetic node which would otherwise be eligible on its own.
define i16 @mixed_cycle(ptr %out) {
; IR-LABEL: define i16 @mixed_cycle(
; IR-NOT: .word
; IR: %a = phi i8
; IR: %b = phi i8
; IR: %next.a = add nuw i8 %b, 1
; IR: %next.b = add i8 %a, 2
; CHECK-LABEL: mixed_cycle:
; CHECK: ret
entry:
  br label %loop
loop:
  %a = phi i8 [250, %entry], [%next.a, %loop]
  %b = phi i8 [251, %entry], [%next.b, %loop]
  store volatile i8 %a, ptr %out
  %next.a = add nuw i8 %b, 1
  %next.b = add i8 %a, 2
  %done = icmp eq i8 %a, 5
  br i1 %done, label %exit, label %loop
exit:
  %result = zext i8 %a to i16
  ret i16 %result
}
