; RUN: llc -mtriple=avm -O2 -verify-machineinstrs < %s | FileCheck %s --check-prefix=ASM
; RUN: llc -mtriple=avm -O2 -print-after=avm-loop-strength-reduce -o /dev/null < %s 2>&1 | FileCheck %s --check-prefix=IR

; InstCombine may sink these increments onto the backedge. Put them next to
; their loads before instruction selection, including 24-bit program pointers.
define i16 @compare_streams(ptr %a, ptr addrspace(1) %b, i16 %size) {
; ASM-LABEL: compare_streams:
; ASM-DAG: ldp8u {{.*}}[q{{[0-9]+}}+]
; ASM-DAG: ld8u {{.*}}[r{{[0-9]+}}+]
; IR-LABEL: define i16 @compare_streams(
; IR: loop:
; IR: %av = load i8, ptr %ap
; IR-NEXT: %an = getelementptr inbounds{{( nuw)?}} i8, ptr %ap, i16 1
; IR: %bv = load i8, ptr addrspace(1) %bp
; IR-NEXT: %bn = getelementptr inbounds{{( nuw)?}} i8, ptr addrspace(1) %bp, i24 1
entry:
  %empty = icmp eq i16 %size, 0
  br i1 %empty, label %done, label %loop
loop:
  %ap = phi ptr [ %a, %entry ], [ %an, %latch ]
  %bp = phi ptr addrspace(1) [ %b, %entry ], [ %bn, %latch ]
  %n = phi i16 [ %size, %entry ], [ %nn, %latch ]
  %av = load i8, ptr %ap, align 1
  %bv = load i8, ptr addrspace(1) %bp, align 1
  %equal = icmp eq i8 %av, %bv
  %nonzero = icmp ne i8 %av, 0
  %cont = and i1 %equal, %nonzero
  br i1 %cont, label %latch, label %difference
latch:
  %an = getelementptr inbounds i8, ptr %ap, i16 1
  %bn = getelementptr inbounds i8, ptr addrspace(1) %bp, i24 1
  %nn = add i16 %n, -1
  %end = icmp eq i16 %nn, 0
  br i1 %end, label %done, label %loop
difference:
  %aw = zext i8 %av to i16
  %bw = zext i8 %bv to i16
  %diff = sub i16 %aw, %bw
  ret i16 %diff
done:
  ret i16 0
}

; A stride larger than the memory access has no postincrement form.
define i8 @skip_bytes(ptr %base, i16 %count) {
; ASM-LABEL: skip_bytes:
; IR-LABEL: define i8 @skip_bytes(
; IR: loop:
; IR: %v = load i8, ptr %p
; IR-NOT: getelementptr
; IR: br i1 %stop
; IR: latch:
; IR: %next = getelementptr i8, ptr %p, i16 3
entry:
  br label %loop
loop:
  %p = phi ptr [ %base, %entry ], [ %next, %latch ]
  %n = phi i16 [ %count, %entry ], [ %nn, %latch ]
  %v = load i8, ptr %p, align 1
  %stop = icmp eq i8 %v, 0
  br i1 %stop, label %done, label %latch
latch:
  %next = getelementptr i8, ptr %p, i16 3
  %nn = add i16 %n, -1
  %end = icmp eq i16 %nn, 0
  br i1 %end, label %done, label %loop
done:
  ret i8 %v
}

; Do not change placement when the old pointer also escapes the loop.
define ptr @find_byte(ptr %base, i8 %needle) {
; ASM-LABEL: find_byte:
; IR-LABEL: define ptr @find_byte(
; IR: loop:
; IR: %v = load i8, ptr %p
; IR-NOT: getelementptr
; IR: br i1 %found
; IR: latch:
; IR: %next = getelementptr i8, ptr %p, i16 1
entry:
  br label %loop
loop:
  %p = phi ptr [ %base, %entry ], [ %next, %latch ]
  %v = load i8, ptr %p, align 1
  %found = icmp eq i8 %v, %needle
  br i1 %found, label %done, label %latch
latch:
  %next = getelementptr i8, ptr %p, i16 1
  br label %loop
done:
  ret ptr %p
}

; The initial pointer stays live outside the loop. Do not force a second RAM
; pointer into the more expensive lower-register postincrement encoding.
define ptr @preserved_base(ptr %base) {
; ASM-LABEL: preserved_base:
; IR-LABEL: define ptr @preserved_base(
; IR: loop:
; IR: %v = load i8, ptr %p
; IR-NOT: getelementptr
; IR: br i1 %stop
; IR: latch:
; IR: %next = getelementptr i8, ptr %p, i16 1
entry:
  br label %loop
loop:
  %p = phi ptr [ %base, %entry ], [ %next, %latch ]
  %v = load i8, ptr %p, align 1
  %stop = icmp eq i8 %v, 0
  br i1 %stop, label %done, label %latch
latch:
  %next = getelementptr i8, ptr %p, i16 1
  br label %loop
done:
  ret ptr %base
}

; The argument has one static use but is reused on each parent iteration.
; Preserve its RAM scan placement just as for a pointer with multiple uses.
define void @nested_scan(ptr %base) {
; ASM-LABEL: nested_scan:
; IR-LABEL: define void @nested_scan(
; IR: inner:
; IR: %v = load i8, ptr %p
; IR-NOT: getelementptr
; IR: br i1 %stop
; IR: inner.latch:
; IR: %next = getelementptr i8, ptr %p, i16 1
entry:
  br label %outer
outer:
  %i = phi i16 [ 0, %entry ], [ %i.next, %outer.latch ]
  br label %inner
inner:
  %p = phi ptr [ %base, %outer ], [ %next, %inner.latch ]
  %v = load i8, ptr %p, align 1
  %stop = icmp eq i8 %v, 0
  br i1 %stop, label %outer.latch, label %inner.latch
inner.latch:
  %next = getelementptr i8, ptr %p, i16 1
  br label %inner
outer.latch:
  %i.next = add i16 %i, 1
  %end = icmp eq i16 %i.next, 4
  br i1 %end, label %exit, label %outer
exit:
  ret void
}

; A derived initial pointer is not tied to an upper argument register. In the
; AS0 flash window, a cold postincrement load can cost more than the original
; displaced load plus increment. Keep the increment on the backedge.
define i16 @derived_initial(ptr %a, ptr %b, i1 %choose) {
; ASM-LABEL: derived_initial:
; IR-LABEL: define i16 @derived_initial(
; IR: loop:
; IR: %v = load i8, ptr %p
; IR-NOT: getelementptr
; IR: br i1 %stop
; IR: latch:
; IR: %next = getelementptr i8, ptr %p, i16 1
entry:
  br i1 %choose, label %first, label %second
first:
  br label %prepare
second:
  br label %prepare
prepare:
  %initial = phi ptr [ %a, %first ], [ %b, %second ]
  br label %loop
loop:
  %p = phi ptr [ %initial, %prepare ], [ %next, %latch ]
  %n = phi i16 [ 0, %prepare ], [ %nn, %latch ]
  %v = load i8, ptr %p, align 1
  %stop = icmp eq i8 %v, 0
  br i1 %stop, label %done, label %latch
latch:
  %next = getelementptr i8, ptr %p, i16 1
  %nn = add i16 %n, 1
  br label %loop
done:
  ret i16 %n
}
