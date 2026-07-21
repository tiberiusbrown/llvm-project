; RUN: opt -mtriple=avm -passes=loop-reduce -S %s -o %t.ll
; RUN: llc -mtriple=avm -O2 -verify-machineinstrs %t.ll -o - \
; RUN:   | FileCheck %s --check-prefix=ASM
; RUN: opt -mtriple=avm -passes=loop-reduce -S %s -o - \
; RUN:   | FileCheck %s --check-prefix=IR

@source = external global [64 x i8], align 1
@destination = external global [64 x i8], align 1
@stream_a = external global [64 x i8], align 1
@stream_b = external global [64 x i8], align 1
@stream_c = external global [64 x i8], align 1
@stream_d = external global [64 x i8], align 1
@float_a = external global [64 x float], align 1
@float_b = external global [64 x float], align 1

define i16 @copy_checksum() {
; ASM-LABEL: copy_checksum:
; ASM: {{ld8u[ \t]+r[0-7], \[r[0-7]\+\]}}
; ASM: {{st8[ \t]+\[r[0-7]\+\], r[0-7]}}
entry:
  br label %loop

loop:
  %index = phi i16 [ 0, %entry ], [ %next, %loop ]
  %sum = phi i16 [ 0, %entry ], [ %sum.next, %loop ]

  %source.ptr =
      getelementptr inbounds [64 x i8], ptr @source, i16 0, i16 %index
  %value = load i8, ptr %source.ptr, align 1

  %destination.ptr =
      getelementptr inbounds [64 x i8], ptr @destination, i16 0, i16 %index
  store i8 %value, ptr %destination.ptr, align 1

  %wide = zext i8 %value to i16
  %sum.next = add i16 %sum, %wide
  %next = add nuw i16 %index, 1
  %done = icmp eq i16 %next, 64
  br i1 %done, label %exit, label %loop

exit:
  ret i16 %sum.next
}

define i16 @three_byte_streams() {
; ASM-LABEL: three_byte_streams:
; ASM: {{ld8u[ \t]+r[0-7], \[r[0-7]\+\]}}
; ASM: {{ld8u[ \t]+r[0-7], \[r[0-7]\+\]}}
; ASM: {{ld8u[ \t]+r[0-7], \[r[0-7]\+\]}}
entry:
  br label %loop

loop:
  %index = phi i16 [ 0, %entry ], [ %next, %loop ]
  %sum = phi i16 [ 0, %entry ], [ %sum.next, %loop ]

  %a.ptr = getelementptr inbounds [64 x i8], ptr @stream_a, i16 0, i16 %index
  %a = load i8, ptr %a.ptr, align 1
  %b.ptr = getelementptr inbounds [64 x i8], ptr @stream_b, i16 0, i16 %index
  %b = load i8, ptr %b.ptr, align 1
  %c.ptr = getelementptr inbounds [64 x i8], ptr @stream_c, i16 0, i16 %index
  %c = load i8, ptr %c.ptr, align 1

  %a.wide = zext i8 %a to i16
  %b.wide = zext i8 %b to i16
  %c.wide = zext i8 %c to i16
  %sum.ab = add i16 %a.wide, %b.wide
  %sum.abc = add i16 %sum.ab, %c.wide
  %sum.next = add i16 %sum, %sum.abc
  %next = add nuw i16 %index, 1
  %done = icmp eq i16 %next, 64
  br i1 %done, label %exit, label %loop

exit:
  ret i16 %sum.next
}

define i16 @four_byte_streams() {
; IR-LABEL: define i16 @four_byte_streams(
; IR-NOT: phi ptr
; IR: phi i16
; IR-NOT: phi ptr
; IR: ret i16
entry:
  br label %loop

loop:
  %index = phi i16 [ 0, %entry ], [ %next, %loop ]
  %sum = phi i16 [ 0, %entry ], [ %sum.next, %loop ]

  %a.ptr = getelementptr inbounds [64 x i8], ptr @stream_a, i16 0, i16 %index
  %a = load i8, ptr %a.ptr, align 1
  %b.ptr = getelementptr inbounds [64 x i8], ptr @stream_b, i16 0, i16 %index
  %b = load i8, ptr %b.ptr, align 1
  %c.ptr = getelementptr inbounds [64 x i8], ptr @stream_c, i16 0, i16 %index
  %c = load i8, ptr %c.ptr, align 1
  %d.ptr = getelementptr inbounds [64 x i8], ptr @stream_d, i16 0, i16 %index
  %d = load i8, ptr %d.ptr, align 1

  %a.wide = zext i8 %a to i16
  %b.wide = zext i8 %b to i16
  %c.wide = zext i8 %c to i16
  %d.wide = zext i8 %d to i16
  %sum.ab = add i16 %a.wide, %b.wide
  %sum.cd = add i16 %c.wide, %d.wide
  %sum.abcd = add i16 %sum.ab, %sum.cd
  %sum.next = add i16 %sum, %sum.abcd
  %next = add nuw i16 %index, 1
  %done = icmp eq i16 %next, 64
  br i1 %done, label %exit, label %loop

exit:
  ret i16 %sum.next
}

define float @two_float_streams() {
; IR-LABEL: define float @two_float_streams(
; IR-NOT: phi ptr
; IR: phi i16
; IR-NOT: phi ptr
; IR: ret float
entry:
  br label %loop

loop:
  %index = phi i16 [ 0, %entry ], [ %next, %loop ]
  %sum = phi float [ 0.000000e+00, %entry ], [ %sum.next, %loop ]

  %a.ptr = getelementptr inbounds [64 x float], ptr @float_a, i16 0, i16 %index
  %a = load float, ptr %a.ptr, align 1
  %b.ptr = getelementptr inbounds [64 x float], ptr @float_b, i16 0, i16 %index
  %b = load float, ptr %b.ptr, align 1

  %values = fadd float %a, %b
  %sum.next = fadd float %sum, %values
  %next = add nuw i16 %index, 1
  %done = icmp eq i16 %next, 64
  br i1 %done, label %exit, label %loop

exit:
  ret float %sum.next
}

define void @runtime_stride_store(ptr %base, i16 %limit, i16 %stride) {
; IR-LABEL: define void @runtime_stride_store(
; IR-NOT: phi ptr
; IR: phi i16
; IR-NOT: phi ptr
; IR: ret void
entry:
  br label %loop

loop:
  %index = phi i16 [ 0, %entry ], [ %next, %loop ]
  %element = getelementptr inbounds i8, ptr %base, i16 %index
  store i8 1, ptr %element, align 1
  %next = add i16 %index, %stride
  %done = icmp ult i16 %next, %limit
  br i1 %done, label %loop, label %exit

exit:
  ret void
}
