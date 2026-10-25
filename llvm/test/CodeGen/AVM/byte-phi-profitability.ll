; RUN: llc -mtriple=avm -O2 -disable-lsr -stop-after=avm-byte-phis %s -o - | FileCheck %s
; RUN: llc -mtriple=avm -O2 -verify-machineinstrs %s -o %t

; Saving normalization at the outer latch does not justify disturbing the
; allocation of a much hotter inner loop. The counter crosses that loop even
; though none of its consumers are in the inner body.
define void @cold_outer_counter(ptr %out) {
; CHECK-LABEL: define void @cold_outer_counter(
; CHECK-NOT: .word
; CHECK: %i = phi i8
; CHECK: %next = add nuw nsw i8 %i, 1
; CHECK: icmp eq i8 %next, 8
; CHECK: ret void
entry:
  br label %outer
outer:
  %i = phi i8 [0, %entry], [%next, %latch]
  br label %inner
inner:
  %j = phi i16 [0, %outer], [%j.next, %inner]
  store volatile i16 %j, ptr %out
  %j.next = add nuw nsw i16 %j, 1
  %inner.done = icmp eq i16 %j.next, 64
  br i1 %inner.done, label %latch, label %inner
latch:
  %next = add nuw nsw i8 %i, 1
  %done = icmp eq i8 %next, 8
  br i1 %done, label %exit, label %outer
exit:
  ret void
}

; Computing the update before entering the inner loop still leaves the next
; induction value live across it. A guard looking only for late uses of %i
; would miss this case.
define void @early_update(ptr %out) {
; CHECK-LABEL: define void @early_update(
; CHECK-NOT: .word
; CHECK: %i = phi i8
; CHECK: %next = add nuw nsw i8 %i, 1
; CHECK: icmp eq i8 %next, 8
; CHECK: ret void
entry:
  br label %outer
outer:
  %i = phi i8 [0, %entry], [%next, %latch]
  %next = add nuw nsw i8 %i, 1
  store volatile i8 %next, ptr %out
  br label %inner
inner:
  %j = phi i16 [0, %outer], [%j.next, %inner]
  store volatile i16 %j, ptr %out
  %j.next = add nuw nsw i16 %j, 1
  %inner.done = icmp eq i16 %j.next, 64
  br i1 %inner.done, label %latch, label %inner
latch:
  %done = icmp eq i8 %next, 8
  br i1 %done, label %exit, label %outer
exit:
  ret void
}

; An unsigned comparison in the hot body gives the outer induction web a
; benefit there. This is not a blanket ban on outer-loop promotion.
define void @hot_outer_consumer(ptr %out, ptr %limits) {
; CHECK-LABEL: define void @hot_outer_consumer(
; CHECK: %i.word = phi i16
; CHECK: %limit = load volatile i8
; CHECK: %limit.word = zext i8 %limit to i16
; CHECK: icmp ult i16 %i.word, %limit.word
; CHECK: %next.word = add nuw i16 %i.word, 1
; CHECK: icmp eq i16 %next.word, 8
; CHECK: ret void
entry:
  br label %outer
outer:
  %i = phi i8 [0, %entry], [%next, %latch]
  br label %inner
inner:
  %j = phi i16 [0, %outer], [%j.next, %inner]
  %limit = load volatile i8, ptr %limits
  %test = icmp ult i8 %i, %limit
  store volatile i1 %test, ptr %out
  %j.next = add nuw nsw i16 %j, 1
  %inner.done = icmp eq i16 %j.next, 64
  br i1 %inner.done, label %latch, label %inner
latch:
  %next = add nuw nsw i8 %i, 1
  %done = icmp eq i8 %next, 8
  br i1 %done, label %exit, label %outer
exit:
  ret void
}

; Follow the dependent PHI web when looking for hot consumers, rather than
; requiring a direct word use of the outer PHI itself.
define void @hot_web_consumer(ptr %out, ptr %p) {
; CHECK-LABEL: define void @hot_web_consumer(
; CHECK: %i.word = phi i16
; CHECK: %byte.word = phi i16 [ %i.word, %outer ], [ %loaded.word, %inner ]
; CHECK: store volatile i16 %byte.word
; CHECK: %next.word = add nuw i16 %i.word, 1
; CHECK: ret void
entry:
  br label %outer
outer:
  %i = phi i8 [0, %entry], [%next, %latch]
  br label %inner
inner:
  %byte = phi i8 [%i, %outer], [%loaded, %inner]
  %j = phi i16 [0, %outer], [%j.next, %inner]
  %word = zext i8 %byte to i16
  store volatile i16 %word, ptr %out
  %loaded = load volatile i8, ptr %p
  %j.next = add nuw nsw i16 %j, 1
  %inner.done = icmp eq i16 %j.next, 64
  br i1 %inner.done, label %latch, label %inner
latch:
  %next = add nuw nsw i8 %i, 1
  %done = icmp eq i8 %next, 8
  br i1 %done, label %exit, label %outer
exit:
  ret void
}

; The nested loop is on an exit-only path. No outer induction state needs to
; survive it, so the cold latch comparison may still be widened.
define void @exit_only_nested_loop(ptr %out, ptr %flag) {
; CHECK-LABEL: define void @exit_only_nested_loop(
; CHECK: %i.word = phi i16
; CHECK: %next.word = add nuw i16 %i.word, 1
; CHECK: icmp eq i16 %next.word, 8
; CHECK: ret void
entry:
  br label %outer
outer:
  %i = phi i8 [0, %entry], [%next, %latch]
  %leave = load volatile i1, ptr %flag
  br i1 %leave, label %inner, label %latch
inner:
  %j = phi i16 [0, %outer], [%j.next, %inner]
  store volatile i16 %j, ptr %out
  %j.next = add nuw nsw i16 %j, 1
  %inner.done = icmp eq i16 %j.next, 64
  br i1 %inner.done, label %exit, label %inner
latch:
  %next = add nuw nsw i8 %i, 1
  %done = icmp eq i8 %next, 8
  br i1 %done, label %exit, label %outer
exit:
  ret void
}

; An innermost induction keeps its per-iteration normalization savings.
define void @inner_counter(ptr %out) {
; CHECK-LABEL: define void @inner_counter(
; CHECK: %i.word = phi i16
; CHECK: store volatile i8
; CHECK: %next.word = add nuw i16 %i.word, 1
; CHECK: icmp eq i16 %next.word, 32
; CHECK: ret void
entry:
  br label %outer
outer:
  %repeat = phi i16 [0, %entry], [%repeat.next, %latch]
  br label %inner
inner:
  %i = phi i8 [0, %outer], [%next, %inner]
  store volatile i8 %i, ptr %out
  %next = add nuw nsw i8 %i, 1
  %inner.done = icmp eq i8 %next, 32
  br i1 %inner.done, label %latch, label %inner
latch:
  %repeat.next = add nuw nsw i16 %repeat, 1
  %done = icmp eq i16 %repeat.next, 8
  br i1 %done, label %exit, label %outer
exit:
  ret void
}
