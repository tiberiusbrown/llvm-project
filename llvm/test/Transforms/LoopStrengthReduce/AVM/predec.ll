; RUN: opt -mtriple=avm -passes=loop-reduce -S %s -o %t.ll
; RUN: llc -mtriple=avm -O2 -verify-machineinstrs %t.ll -o - \
; RUN:   | FileCheck %s --check-prefix=ASM
; RUN: opt -mtriple=avm -passes=loop-reduce -S %s -o - \
; RUN:   | FileCheck %s --check-prefix=IR

@source = external global [64 x i8], align 1
@destination = external global [64 x i8], align 1
@word_source = external global [32 x i16], align 1
@stream_a = external global [64 x i8], align 1
@stream_b = external global [64 x i8], align 1
@stream_c = external global [64 x i8], align 1
@stream_d = external global [64 x i8], align 1
@program_source = external addrspace(1) global [64 x i8], align 1

define i16 @reverse_copy() {
; IR-LABEL: define i16 @reverse_copy(
; IR: phi ptr
; IR: phi ptr
; ASM-LABEL: reverse_copy:
; ASM: dec16 [[SRC:r[0-7]]]
; ASM-NEXT: ld8u {{r[0-7]}}, {{\[}}[[SRC]]{{\]}}
; ASM: dec16 [[DST:r[0-7]]]
; ASM-NEXT: st8 {{\[}}[[DST]]{{\]}}, {{r[0-7]}}
entry:
  br label %loop

loop:
  %source.current = phi ptr [ getelementptr ([64 x i8], ptr @source, i16 0, i16 64), %entry ], [ %source.next, %loop ]
  %destination.current = phi ptr [ getelementptr ([64 x i8], ptr @destination, i16 0, i16 64), %entry ], [ %destination.next, %loop ]
  %remaining = phi i16 [ 64, %entry ], [ %remaining.next, %loop ]
  %sum = phi i16 [ 0, %entry ], [ %sum.next, %loop ]
  %source.next = getelementptr inbounds i8, ptr %source.current, i16 -1
  %value = load i8, ptr %source.next, align 1
  %destination.next =
      getelementptr inbounds i8, ptr %destination.current, i16 -1
  store i8 %value, ptr %destination.next, align 1
  %wide = zext i8 %value to i16
  %sum.next = add i16 %sum, %wide
  %remaining.next = add nsw i16 %remaining, -1
  %done = icmp eq i16 %remaining.next, 0
  br i1 %done, label %exit, label %loop

exit:
  ret i16 %sum.next
}

define i16 @reverse_words() {
; IR-LABEL: define i16 @reverse_words(
; IR: phi ptr
; ASM-LABEL: reverse_words:
; ASM: addi.s8 [[PTR:r[0-7]]], -2
; ASM-NEXT: ld16 {{r[0-7]}}, {{\[}}[[PTR]]{{\]}}
entry:
  br label %loop
loop:
  %current = phi ptr [ getelementptr ([32 x i16], ptr @word_source, i16 0, i16 32), %entry ], [ %next, %loop ]
  %remaining = phi i16 [ 32, %entry ], [ %remaining.next, %loop ]
  %sum = phi i16 [ 0, %entry ], [ %sum.next, %loop ]
  %next = getelementptr inbounds i16, ptr %current, i16 -1
  %value = load i16, ptr %next, align 1
  %sum.next = add i16 %sum, %value
  %remaining.next = add nsw i16 %remaining, -1
  %done = icmp eq i16 %remaining.next, 0
  br i1 %done, label %exit, label %loop
exit:
  ret i16 %sum.next
}

define i16 @four_reverse_streams() {
; IR-LABEL: define i16 @four_reverse_streams(
; IR-NOT: phi ptr
; IR: phi i16
; IR-NOT: phi ptr
entry:
  br label %loop
loop:
  %index = phi i16 [ 64, %entry ], [ %next, %loop ]
  %next = add nsw i16 %index, -1
  %a.ptr = getelementptr inbounds [64 x i8], ptr @stream_a, i16 0, i16 %next
  %b.ptr = getelementptr inbounds [64 x i8], ptr @stream_b, i16 0, i16 %next
  %c.ptr = getelementptr inbounds [64 x i8], ptr @stream_c, i16 0, i16 %next
  %d.ptr = getelementptr inbounds [64 x i8], ptr @stream_d, i16 0, i16 %next
  %a = load i8, ptr %a.ptr, align 1
  %b = load i8, ptr %b.ptr, align 1
  %c = load i8, ptr %c.ptr, align 1
  %d = load i8, ptr %d.ptr, align 1
  %ab = add i8 %a, %b
  %cd = add i8 %c, %d
  %all = add i8 %ab, %cd
  %done = icmp eq i16 %next, 0
  br i1 %done, label %exit, label %loop
exit:
  %result = zext i8 %all to i16
  ret i16 %result
}

define i16 @mixed_directions() {
; IR-LABEL: define i16 @mixed_directions(
; IR: phi i16
; IR: phi ptr
; IR-NOT: phi ptr
entry:
  br label %loop
loop:
  %index = phi i16 [ 64, %entry ], [ %next, %loop ]
  %next = add nsw i16 %index, -1
  %forward.index = sub i16 64, %index
  %a.ptr = getelementptr inbounds [64 x i8], ptr @stream_a, i16 0, i16 %next
  %b.ptr =
      getelementptr inbounds [64 x i8], ptr @stream_b, i16 0, i16 %forward.index
  %a = load i8, ptr %a.ptr, align 1
  %b = load i8, ptr %b.ptr, align 1
  %sum = add i8 %a, %b
  %done = icmp eq i16 %next, 0
  br i1 %done, label %exit, label %loop
exit:
  %result = zext i8 %sum to i16
  ret i16 %result
}

define i16 @program_reverse() {
; IR-LABEL: define i16 @program_reverse(
; IR-NOT: phi ptr addrspace(1)
; IR: phi i16
; IR-NOT: phi ptr addrspace(1)
entry:
  br label %loop
loop:
  %index = phi i16 [ 64, %entry ], [ %next, %loop ]
  %sum = phi i16 [ 0, %entry ], [ %sum.next, %loop ]
  %next = add nsw i16 %index, -1
  %ptr = getelementptr inbounds [64 x i8], ptr addrspace(1) @program_source,
                                i16 0, i16 %next
  %value = load i8, ptr addrspace(1) %ptr, align 1
  %wide = zext i8 %value to i16
  %sum.next = add i16 %sum, %wide
  %done = icmp eq i16 %next, 0
  br i1 %done, label %exit, label %loop
exit:
  ret i16 %sum.next
}
