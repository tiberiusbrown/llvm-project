; RUN: opt -mtriple=avm -passes=loop-reduce -S %s -o %t.ll
; RUN: llc -mtriple=avm -O2 -verify-machineinstrs %t.ll -o - \
; RUN:   | FileCheck %s --check-prefix=ASM

@source = external global [64 x i8], align 1
@destination = external global [64 x i8], align 1

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
