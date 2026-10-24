; RUN: llc -mtriple=avm -O2 -verify-machineinstrs %s -o - | FileCheck %s
; RUN: llc -mtriple=avm -O2 -stop-after=avm-loop-strength-reduce %s -o - | FileCheck %s --check-prefix=IR
; RUN: llc -mtriple=avm -O2 -stop-after=finalize-isel %s -o - | FileCheck %s --check-prefix=ISEL

; The slot remains live for output, so the pointer is an additional IV.
; Generic LSR must not treat base+index as a free AVM memory operand.
define i16 @scan_records(ptr %base, ptr %order) {
; CHECK-LABEL: scan_records:
; CHECK-NOT: mul
; CHECK: ld8u
; CHECK-NOT: mul
; CHECK: addi.s8 {{r[0-9]+}}, 18
; CHECK-NOT: mul
; CHECK: ret
entry:
  br label %loop
loop:
  %i = phi i16 [0, %entry], [%next, %latch]
  %count = phi i16 [0, %entry], [%result, %latch]
  %offset = mul nuw i16 %i, 18
  %record = getelementptr inbounds i8, ptr %base, i16 %offset
  %field = getelementptr inbounds i8, ptr %record, i16 10
  %id = load i8, ptr %field
  %active = icmp ne i8 %id, 0
  br i1 %active, label %body, label %latch
body:
  %slot = trunc i16 %i to i8
  %out = getelementptr i8, ptr %order, i16 %count
  store i8 %slot, ptr %out
  %added = add i16 %count, 1
  br label %latch
latch:
  %result = phi i16 [%count, %loop], [%added, %body]
  %next = add nuw nsw i16 %i, 1
  %done = icmp eq i16 %next, 8
  br i1 %done, label %exit, label %loop
exit:
  ret i16 %result
}

; Indirect inner accesses must not block reduction of the affine outer scan.
; Two fields share a single pointer recurrence, using displaced loads.
define i16 @nested_records(ptr %base, ptr %order, i16 %n) {
; IR-LABEL: define i16 @nested_records(
; IR: phi ptr
; IR: avm.field = getelementptr i8, ptr %{{[^,]+}}, i16 0
; IR: avm.field{{[0-9]*}} = getelementptr i8, ptr %{{[^,]+}}, i16 -8
; CHECK-LABEL: nested_records:
; CHECK: ld8u
; CHECK: ld16
; CHECK: addi.s8 {{r[0-9]+}}, 18
; CHECK: ret
entry:
  br label %outer
outer:
  %i = phi i16 [0, %entry], [%next, %latch]
  %sum = phi i16 [0, %entry], [%result, %latch]
  %offset = mul nuw i16 %i, 18
  %record = getelementptr inbounds i8, ptr %base, i16 %offset
  %field = getelementptr inbounds i8, ptr %record, i16 10
  %id = load i8, ptr %field
  %active = icmp ne i8 %id, 0
  br i1 %active, label %body, label %latch
body:
  %yfield = getelementptr inbounds i8, ptr %record, i16 2
  %y = load i16, ptr %yfield
  br label %inner
inner:
  %j = phi i16 [%n, %body], [%prev, %inner]
  %prev = sub i16 %j, 1
  %p = getelementptr i8, ptr %order, i16 %prev
  %value = load volatile i8, ptr %p
  %done.inner = icmp eq i8 %value, %id
  br i1 %done.inner, label %after, label %inner
after:
  %added = add i16 %sum, %y
  %slot.added = add i16 %added, %i
  br label %latch
latch:
  %result = phi i16 [%sum, %outer], [%slot.added, %after]
  %next = add nuw nsw i16 %i, 1
  %done = icmp eq i16 %next, 8
  br i1 %done, label %exit, label %outer
exit:
  ret i16 %result
}

declare void @consume(i16, i16, i16)

; No affine address to reduce: retain the product and preserve the proven
; range of the word PHI through DAG import. The inner loop prevents generic
; LSR from replacing the outer multiply with a separate recurrence.
define void @bounded_multiply(i16 %n) {
; CHECK-LABEL: bounded_multiply:
; CHECK: {{mulu8.w|mul16}}
; CHECK: ret
; ISEL-LABEL: name: bounded_multiply
; ISEL: MULU8W_FLEX_PSEUDO
; ISEL-NOT: MUL16_PSEUDO
; ISEL: RET_PSEUDO
entry:
  br label %outer
outer:
  %i = phi i16 [0, %entry], [%next, %latch]
  %product = mul nuw i16 %i, 18
  br label %inner
inner:
  %j = phi i16 [0, %outer], [%j.next, %inner]
  call void @consume(i16 %i, i16 %product, i16 %j)
  %j.next = add i16 %j, 1
  %done.inner = icmp eq i16 %j.next, %n
  br i1 %done.inner, label %latch, label %inner
latch:
  %next = add nuw nsw i16 %i, 1
  %done = icmp eq i16 %next, 8
  br i1 %done, label %exit, label %outer
exit:
  ret void
}

; A small trip count is not sufficient: these indices exceed a byte.
define void @wide_multiply(i16 %n) {
; CHECK-LABEL: wide_multiply:
; CHECK-NOT: mulu8.w
; CHECK: mul16
; CHECK: ret
; ISEL-LABEL: name: wide_multiply
; ISEL-NOT: MULU8W_FLEX_PSEUDO
; ISEL: MUL16_PSEUDO
entry:
  br label %outer
outer:
  %i = phi i16 [254, %entry], [%next, %latch]
  %product = mul i16 %i, 18
  br label %inner
inner:
  %j = phi i16 [0, %outer], [%j.next, %inner]
  call void @consume(i16 %i, i16 %product, i16 %j)
  %j.next = add i16 %j, 1
  %done.inner = icmp eq i16 %j.next, %n
  br i1 %done.inner, label %latch, label %inner
latch:
  %next = add nuw nsw i16 %i, 1
  %done = icmp eq i16 %next, 259
  br i1 %done, label %exit, label %outer
exit:
  ret void
}
