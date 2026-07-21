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
@wide_stream = external global [8 x i32], align 1
@byte_stream = external global [8 x i8], align 1

%particle = type { i32, i16, i8, i8 }
@particles = external global [24 x %particle], align 1

@ring_a = external global [64 x i8], align 1
@ring_b = external global [64 x i8], align 1
@ring_words = external global [64 x i16], align 1

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

define i32 @wide_and_byte_streams() {
; ASM-LABEL: wide_and_byte_streams:
; ASM: {{ld8u[ \t]+r[0-7], \[r[0-7]\+\]}}
; IR-LABEL: define i32 @wide_and_byte_streams(
; IR: phi ptr
entry:
  br label %loop

loop:
  %index = phi i16 [ 0, %entry ], [ %next, %loop ]
  %sum = phi i32 [ 0, %entry ], [ %sum.next, %loop ]

  %wide.ptr =
      getelementptr inbounds [8 x i32], ptr @wide_stream, i16 0, i16 %index
  %wide = load i32, ptr %wide.ptr, align 1

  %byte.ptr =
      getelementptr inbounds [8 x i8], ptr @byte_stream, i16 0, i16 %index
  %byte = load i8, ptr %byte.ptr, align 1
  %byte.wide = zext i8 %byte to i32

  %partial = add i32 %sum, %wide
  %sum.next = add i32 %partial, %byte.wide

  %next = add nuw i16 %index, 1
  %done = icmp eq i16 %next, 8
  br i1 %done, label %exit, label %loop

exit:
  ret i32 %sum.next
}

define i32 @aggregate_fields() {
; IR-LABEL: define i32 @aggregate_fields(
; IR: phi ptr
; IR-NOT: phi ptr
; IR: ret i32
; ASM-LABEL: aggregate_fields:
; ASM: {{addi\.s8[ \t]+r[0-7], (0x)?8}}
entry:
  br label %loop

loop:
  %index = phi i16 [ 0, %entry ], [ %next, %loop ]
  %sum = phi i32 [ 0, %entry ], [ %sum.next, %loop ]

  %item =
      getelementptr inbounds [24 x %particle], ptr @particles,
                             i16 0, i16 %index
  %position.ptr =
      getelementptr inbounds %particle, ptr %item, i32 0, i32 0
  %velocity.ptr =
      getelementptr inbounds %particle, ptr %item, i32 0, i32 1
  %flags.ptr =
      getelementptr inbounds %particle, ptr %item, i32 0, i32 2
  %delta.ptr =
      getelementptr inbounds %particle, ptr %item, i32 0, i32 3

  %position = load i32, ptr %position.ptr, align 1
  %velocity = load i16, ptr %velocity.ptr, align 1
  %flags = load i8, ptr %flags.ptr, align 1
  %delta = load i8, ptr %delta.ptr, align 1

  %velocity.wide = zext i16 %velocity to i32
  %flags.wide = zext i8 %flags to i32
  %delta.wide = sext i8 %delta to i32

  %sum.position = add i32 %sum, %position
  %sum.velocity = add i32 %sum.position, %velocity.wide
  %sum.flags = add i32 %sum.velocity, %flags.wide
  %sum.next = add i32 %sum.flags, %delta.wide

  %next = add nuw i16 %index, 1
  %done = icmp eq i16 %next, 24
  br i1 %done, label %exit, label %loop

exit:
  ret i32 %sum.next
}

define i16 @wrapped_index_streams() {
; IR-LABEL: define i16 @wrapped_index_streams(
; IR: phi ptr
; IR-NOT: phi ptr
; IR: and i16
; IR: and i16
; IR: ret i16
entry:
  br label %loop

loop:
  %index = phi i16 [ 0, %entry ], [ %next, %loop ]
  %sum = phi i16 [ 0, %entry ], [ %sum.next, %loop ]

  %index1.raw = add i16 %index, 1
  %index1 = and i16 %index1.raw, 63
  %index3.raw = add i16 %index, 3
  %index3 = and i16 %index3.raw, 63
  %index5.raw = add i16 %index, 5
  %index5 = and i16 %index5.raw, 63

  %a0.ptr =
      getelementptr inbounds [64 x i8], ptr @ring_a, i16 0, i16 %index
  %a0 = load i8, ptr %a0.ptr, align 1
  %a1.ptr =
      getelementptr inbounds [64 x i8], ptr @ring_a, i16 0, i16 %index1
  %a1 = load i8, ptr %a1.ptr, align 1

  %b0.ptr =
      getelementptr inbounds [64 x i8], ptr @ring_b, i16 0, i16 %index
  %b0 = load i8, ptr %b0.ptr, align 1
  %b3.ptr =
      getelementptr inbounds [64 x i8], ptr @ring_b, i16 0, i16 %index3
  %b3 = load i8, ptr %b3.ptr, align 1

  %w0.ptr =
      getelementptr inbounds [64 x i16], ptr @ring_words, i16 0, i16 %index
  %w0 = load i16, ptr %w0.ptr, align 1
  %w5.ptr =
      getelementptr inbounds [64 x i16], ptr @ring_words, i16 0, i16 %index5
  %w5 = load i16, ptr %w5.ptr, align 1

  %a0.wide = zext i8 %a0 to i16
  %a1.wide = zext i8 %a1 to i16
  %b0.wide = zext i8 %b0 to i16
  %b3.wide = zext i8 %b3 to i16

  %sum.a0 = add i16 %sum, %a0.wide
  %sum.a1 = add i16 %sum.a0, %a1.wide
  %sum.b0 = add i16 %sum.a1, %b0.wide
  %sum.b3 = add i16 %sum.b0, %b3.wide
  %sum.w0 = add i16 %sum.b3, %w0
  %sum.next = add i16 %sum.w0, %w5

  %next = add nuw i16 %index, 1
  %done = icmp eq i16 %next, 64
  br i1 %done, label %exit, label %loop

exit:
  ret i16 %sum.next
}
