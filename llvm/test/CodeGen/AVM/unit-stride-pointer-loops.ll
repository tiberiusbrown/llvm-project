; RUN: llc -mtriple=avm -O2 -verify-machineinstrs < %s | FileCheck %s --check-prefix=ASM
; RUN: llc -mtriple=avm -O2 -print-after=avm-unit-stride-loops -o /dev/null < %s 2>&1 | FileCheck %s --check-prefix=IR
; RUN: llc -mtriple=avm -O2 -disable-lsr -print-after=avm-unit-stride-loops -o /dev/null < %s 2>&1 | FileCheck %s --check-prefix=WRAP

@a = internal global [32 x i8] zeroinitializer, align 1
@b = internal global [32 x i8] zeroinitializer, align 1
@c = internal global [32 x i8] zeroinitializer, align 1
@d = internal global [32 x i8] zeroinitializer, align 1

; Late global merging exposes four arrays as constant-offset fields of one
; unit-stride stream. Replace the index and repeated base addition with one
; pointer IV, and compare its next value to the invariant end pointer.
define i16 @four_arrays() {
; ASM-LABEL: four_arrays:
; ASM: ld8u
; ASM: ld8u
; ASM: ld8u
; ASM: ld8u
; ASM: inc16
; ASM: cmp
; ASM: ret
; IR-LABEL: define i16 @four_arrays(
; IR: loop:
; IR-NOT: %i = phi
; IR: %avm.stream.ptr = phi ptr
; IR: %avm.field = getelementptr i8, ptr %avm.stream.ptr, i16 0
; IR-NEXT: load volatile i8, ptr %avm.field
; IR: %avm.stream.next = getelementptr i8, ptr %avm.stream.ptr, i16 1
; IR: icmp {{eq|ne}} ptr %avm.stream.next,
; IR: exit:
entry:
  br label %loop
loop:
  %i = phi i16 [ 0, %entry ], [ %next, %loop ]
  %sum = phi i16 [ 0, %entry ], [ %total, %loop ]
  %ap = getelementptr [32 x i8], ptr @a, i16 0, i16 %i
  %av = load volatile i8, ptr %ap, align 1
  %bp = getelementptr [32 x i8], ptr @b, i16 0, i16 %i
  %bv = load volatile i8, ptr %bp, align 1
  %cp = getelementptr [32 x i8], ptr @c, i16 0, i16 %i
  %cv = load volatile i8, ptr %cp, align 1
  %dp = getelementptr [32 x i8], ptr @d, i16 0, i16 %i
  %dv = load volatile i8, ptr %dp, align 1
  %aw = zext i8 %av to i16
  %bw = zext i8 %bv to i16
  %cw = zext i8 %cv to i16
  %dw = zext i8 %dv to i16
  %ab = add i16 %aw, %bw
  %cd = add i16 %cw, %dw
  %all = add i16 %ab, %cd
  %total = add i16 %sum, %all
  %next = add nuw nsw i16 %i, 1
  %done = icmp eq i16 %next, 32
  br i1 %done, label %exit, label %loop
exit:
  ret i16 %total
}

; The integer counter also contributes to the result. Replacing it would add
; a pointer without eliminating the counter, so retain the original loop.
define i16 @live_counter(ptr %base) {
; ASM-LABEL: live_counter:
; IR-LABEL: define i16 @live_counter(
; IR-NOT: avm.stream.ptr
; IR: %i = phi i16
; IR: %next = add nuw nsw i16 %i, 1
; IR: exit:
entry:
  br label %loop
loop:
  %i = phi i16 [ 0, %entry ], [ %next, %loop ]
  %sum = phi i16 [ 0, %entry ], [ %total, %loop ]
  %p = getelementptr i8, ptr %base, i16 %i
  %a0 = load volatile i8, ptr %p, align 1
  %p1 = getelementptr i8, ptr %p, i16 32
  %a1 = load volatile i8, ptr %p1, align 1
  %p2 = getelementptr i8, ptr %p, i16 64
  %a2 = load volatile i8, ptr %p2, align 1
  %w0 = zext i8 %a0 to i16
  %w1 = zext i8 %a1 to i16
  %w2 = zext i8 %a2 to i16
  %s0 = add i16 %sum, %i
  %s1 = add i16 %s0, %w0
  %s2 = add i16 %s1, %w1
  %total = add i16 %s2, %w2
  %next = add nuw nsw i16 %i, 1
  %done = icmp eq i16 %next, 32
  br i1 %done, label %exit, label %loop
exit:
  ret i16 %total
}

; A stride-two address wraps after 32768 iterations, before this loop ends.
; Comparing the next pointer to base+65538 would stop after the first iteration.
define i16 @wrapping_address(ptr %base) {
; ASM-LABEL: wrapping_address:
; WRAP-LABEL: define i16 @wrapping_address(
; WRAP-NOT: avm.stream.ptr
; WRAP: %i = phi i16
; WRAP: %next = add i16 %i, 1
; WRAP: exit:
entry:
  br label %loop
loop:
  %i = phi i16 [ 0, %entry ], [ %next, %loop ]
  %sum = phi i16 [ 0, %entry ], [ %total, %loop ]
  %offset = shl i16 %i, 1
  %p = getelementptr i8, ptr %base, i16 %offset
  %a0 = load volatile i8, ptr %p, align 1
  %p1 = getelementptr i8, ptr %p, i16 16
  %a1 = load volatile i8, ptr %p1, align 1
  %p2 = getelementptr i8, ptr %p, i16 32
  %a2 = load volatile i8, ptr %p2, align 1
  %w0 = zext i8 %a0 to i16
  %w1 = zext i8 %a1 to i16
  %w2 = zext i8 %a2 to i16
  %s0 = add i16 %sum, %w0
  %s1 = add i16 %s0, %w1
  %total = add i16 %s1, %w2
  %next = add i16 %i, 1
  %done = icmp eq i16 %next, 32769
  br i1 %done, label %exit, label %loop
exit:
  ret i16 %total
}
