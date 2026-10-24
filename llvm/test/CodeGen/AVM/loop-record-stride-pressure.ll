; RUN: llc -mtriple=avm -O2 -stop-after=avm-loop-strength-reduce %s -o - | FileCheck %s
; RUN: llc -mtriple=avm -O2 -verify-machineinstrs %s -o /dev/null

declare void @consume(i16, i16, i16, i16)

; Five loop-carried words leave too little room for an additional pointer
; alongside invariants and the nested loop. The range fallback still applies.
define i16 @crowded_scan(ptr %base, i16 %n, i16 %seed) {
; CHECK-LABEL: define i16 @crowded_scan(
; CHECK-NOT: phi ptr
; CHECK: %offset = mul nuw i16 %i, 18
; CHECK-NOT: phi ptr
; CHECK: ret i16
entry:
  br label %outer
outer:
  %i = phi i16 [0, %entry], [%next, %latch]
  %a = phi i16 [%seed, %entry], [%a.next, %latch]
  %b = phi i16 [2, %entry], [%b.next, %latch]
  %c = phi i16 [3, %entry], [%c.next, %latch]
  %d = phi i16 [4, %entry], [%d.next, %latch]
  %offset = mul nuw i16 %i, 18
  %field = getelementptr inbounds i8, ptr %base, i16 %offset
  %value = load i16, ptr %field
  br label %inner
inner:
  %j = phi i16 [0, %outer], [%j.next, %inner]
  call void @consume(i16 %a, i16 %b, i16 %c, i16 %d)
  %j.next = add i16 %j, 1
  %done.inner = icmp eq i16 %j.next, %n
  br i1 %done.inner, label %latch, label %inner
latch:
  %a.next = add i16 %a, %value
  %b.next = xor i16 %b, %value
  %c.next = sub i16 %c, %value
  %d.next = mul i16 %d, %value
  %next = add nuw nsw i16 %i, 1
  %done = icmp eq i16 %next, 8
  br i1 %done, label %exit, label %outer
exit:
  ret i16 %a.next
}

; Descending outer recurrences use an explicit negative stride and still share
; their base pointer. Value ranges are proven independently of the stride sign.
define i16 @reverse_records(ptr %base, i16 %n) {
; CHECK-LABEL: define i16 @reverse_records(
; CHECK: phi ptr
; CHECK: getelementptr i8, ptr %{{[^,]+}}, i16 -18
; CHECK: ret i16
entry:
  br label %outer
outer:
  %i = phi i16 [7, %entry], [%prev, %latch]
  %sum = phi i16 [0, %entry], [%sum.next, %latch]
  %offset = mul i16 %i, 18
  %field = getelementptr inbounds i8, ptr %base, i16 %offset
  %value = load i16, ptr %field
  br label %inner
inner:
  %j = phi i16 [0, %outer], [%j.next, %inner]
  call void @consume(i16 %sum, i16 %i, i16 %value, i16 %j)
  %j.next = add i16 %j, 1
  %done.inner = icmp eq i16 %j.next, %n
  br i1 %done.inner, label %latch, label %inner
latch:
  %sum.next = add i16 %sum, %value
  %prev = add nsw i16 %i, -1
  %done = icmp eq i16 %i, 0
  br i1 %done, label %exit, label %outer
exit:
  ret i16 %sum.next
}

; Converting only the narrow fields of a record would keep its old wide-field
; address recurrence alive, increasing pressure around pair operations.
define i16 @mixed_width_records(ptr %base, i16 %n) {
; CHECK-LABEL: define i16 @mixed_width_records(
; CHECK-NOT: phi ptr
; CHECK: load volatile i32
; CHECK-NOT: phi ptr
; CHECK: ret i16
entry:
  br label %outer
outer:
  %i = phi i16 [0, %entry], [%next, %latch]
  %sum = phi i16 [0, %entry], [%sum.next, %latch]
  %offset = mul nuw i16 %i, 18
  %field = getelementptr inbounds i8, ptr %base, i16 %offset
  %byte = load volatile i8, ptr %field
  %wide.field = getelementptr inbounds i8, ptr %field, i16 1
  %wide = load volatile i32, ptr %wide.field
  %value = trunc i32 %wide to i16
  %byte.word = zext i8 %byte to i16
  br label %inner
inner:
  %j = phi i16 [0, %outer], [%j.next, %inner]
  call void @consume(i16 %sum, i16 %i, i16 %value, i16 %byte.word)
  %j.next = add i16 %j, 1
  %done.inner = icmp eq i16 %j.next, %n
  br i1 %done.inner, label %latch, label %inner
latch:
  %sum.next = add i16 %sum, %value
  %next = add nuw nsw i16 %i, 1
  %done = icmp eq i16 %next, 8
  br i1 %done, label %exit, label %outer
exit:
  ret i16 %sum.next
}
