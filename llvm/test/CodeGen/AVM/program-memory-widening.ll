; RUN: llc -mtriple=avm-unknown-arduboyfx -mcpu=avm1 \
; RUN:   -mtune=avm-interpreter-32u4-v1 -O2 -verify-machineinstrs < %s \
; RUN:   | FileCheck %s --check-prefix=WIDE
; RUN: llc -mtriple=avm-unknown-arduboyfx -mcpu=avm1 \
; RUN:   -mtune=avm-interpreter-32u4-v1 -O2 -verify-machineinstrs \
; RUN:   -avm-disable-program-memory-widening < %s \
; RUN:   | FileCheck %s --check-prefix=SCALAR

declare void @intervening_call()

define i16 @runtime_byte_stream(ptr addrspace(1) %base, i16 %count) {
; WIDE-LABEL: runtime_byte_stream:
; WIDE-DAG:   ldp32 {{.*}}[q{{[0-9]+}}+]
; WIDE-DAG:   ldp16 {{.*}}[q{{[0-9]+}}+]
; WIDE-DAG:   ldp8u
; SCALAR-LABEL: runtime_byte_stream:
; SCALAR-NOT: ldp16
; SCALAR-NOT: ldp32
; SCALAR:     ldp8u {{.*}}[q{{[0-9]+}}+]
entry:
  br label %loop

loop:
  %p = phi ptr addrspace(1) [ %base, %entry ], [ %next, %loop ]
  %sum = phi i16 [ 0, %entry ], [ %sum.next, %loop ]
  %i = phi i16 [ 0, %entry ], [ %inc, %loop ]
  %value = load i8, ptr addrspace(1) %p, align 1
  %extended = zext i8 %value to i16
  %sum.next = add i16 %sum, %extended
  %next = getelementptr i8, ptr addrspace(1) %p, i32 1
  %inc = add i16 %i, 1
  %done = icmp eq i16 %inc, %count
  br i1 %done, label %exit, label %loop

exit:
  %result = phi i16 [ %sum.next, %loop ]
  ret i16 %result
}

define i16 @runtime_word_stream(ptr addrspace(1) %base, i16 %count) {
; WIDE-LABEL: runtime_word_stream:
; WIDE:       ldp32 {{.*}}[q{{[0-9]+}}+]
; WIDE:       ldp16
; SCALAR-LABEL: runtime_word_stream:
; SCALAR-NOT: ldp32
; SCALAR:     ldp16 {{.*}}[q{{[0-9]+}}+]
entry:
  br label %loop

loop:
  %p = phi ptr addrspace(1) [ %base, %entry ], [ %next, %loop ]
  %sum = phi i16 [ 0, %entry ], [ %sum.next, %loop ]
  %i = phi i16 [ 0, %entry ], [ %inc, %loop ]
  %value = load i16, ptr addrspace(1) %p, align 1
  %sum.next = add i16 %sum, %value
  %next = getelementptr i16, ptr addrspace(1) %p, i32 1
  %inc = add i16 %i, 1
  %done = icmp eq i16 %inc, %count
  br i1 %done, label %exit, label %loop

exit:
  %result = phi i16 [ %sum.next, %loop ]
  ret i16 %result
}

define i16 @explicit_adjacent_byte_pair(ptr addrspace(1) %base, i16 %count) {
; WIDE-LABEL: explicit_adjacent_byte_pair:
; WIDE:       ldp16 {{.*}}[q{{[0-9]+}}+]
; WIDE-NOT:   ldp8u
; SCALAR-LABEL: explicit_adjacent_byte_pair:
; SCALAR:     ldp8u {{.*}}[q{{[0-9]+}}+]
; SCALAR:     ldp8u
entry:
  br label %loop

loop:
  %p = phi ptr addrspace(1) [ %base, %entry ], [ %next, %loop ]
  %sum = phi i16 [ 0, %entry ], [ %sum.next, %loop ]
  %i = phi i16 [ 0, %entry ], [ %inc, %loop ]
  %first = load i8, ptr addrspace(1) %p, align 1
  %second.p = getelementptr i8, ptr addrspace(1) %p, i32 1
  %second = load i8, ptr addrspace(1) %second.p, align 1
  %a = zext i8 %first to i16
  %b = zext i8 %second to i16
  %pair = add i16 %a, %b
  %sum.next = add i16 %sum, %pair
  %next = getelementptr i8, ptr addrspace(1) %p, i32 2
  %inc = add i16 %i, 1
  %done = icmp eq i16 %inc, %count
  br i1 %done, label %exit, label %loop

exit:
  %result = phi i16 [ %sum.next, %loop ]
  ret i16 %result
}

define i16 @fixed_four_byte_stream(ptr addrspace(1) %base) {
; WIDE-LABEL: fixed_four_byte_stream:
; WIDE:       ldp32
; WIDE-NOT:   ldp8u
; SCALAR-LABEL: fixed_four_byte_stream:
; SCALAR-NOT: ldp16
; SCALAR-NOT: ldp32
; SCALAR:     ldp8u {{.*}}[q{{[0-9]+}}+]
entry:
  br label %loop

loop:
  %p = phi ptr addrspace(1) [ %base, %entry ], [ %next, %loop ]
  %sum = phi i16 [ 0, %entry ], [ %sum.next, %loop ]
  %i = phi i16 [ 0, %entry ], [ %inc, %loop ]
  %value = load i8, ptr addrspace(1) %p, align 1
  %extended = zext i8 %value to i16
  %sum.next = add i16 %sum, %extended
  %next = getelementptr i8, ptr addrspace(1) %p, i32 1
  %inc = add i16 %i, 1
  %done = icmp eq i16 %inc, 8
  br i1 %done, label %exit, label %loop

exit:
  %result = phi i16 [ %sum.next, %loop ]
  ret i16 %result
}

define i16 @misaligned_byte_stream(ptr addrspace(1) %base, i16 %count) {
; WIDE-LABEL: misaligned_byte_stream:
; WIDE:       ldp32
entry:
  %misaligned = getelementptr i8, ptr addrspace(1) %base, i32 1
  br label %loop

loop:
  %p = phi ptr addrspace(1) [ %misaligned, %entry ], [ %next, %loop ]
  %sum = phi i16 [ 0, %entry ], [ %sum.next, %loop ]
  %i = phi i16 [ 0, %entry ], [ %inc, %loop ]
  %value = load i8, ptr addrspace(1) %p, align 1
  %extended = zext i8 %value to i16
  %sum.next = add i16 %sum, %extended
  %next = getelementptr i8, ptr addrspace(1) %p, i32 1
  %inc = add i16 %i, 1
  %done = icmp eq i16 %inc, %count
  br i1 %done, label %exit, label %loop

exit:
  %result = phi i16 [ %sum.next, %loop ]
  ret i16 %result
}

define i16 @signed_byte_stream(ptr addrspace(1) %base, i16 %count) {
; WIDE-LABEL: signed_byte_stream:
; WIDE:       ldp32
entry:
  br label %loop

loop:
  %p = phi ptr addrspace(1) [ %base, %entry ], [ %next, %loop ]
  %sum = phi i16 [ 0, %entry ], [ %sum.next, %loop ]
  %i = phi i16 [ 0, %entry ], [ %inc, %loop ]
  %value = load i8, ptr addrspace(1) %p, align 1
  %extended = sext i8 %value to i16
  %sum.next = add i16 %sum, %extended
  %next = getelementptr i8, ptr addrspace(1) %p, i32 1
  %inc = add i16 %i, 1
  %done = icmp eq i16 %inc, %count
  br i1 %done, label %exit, label %loop

exit:
  %result = phi i16 [ %sum.next, %loop ]
  ret i16 %result
}

define i16 @arbitrary_byte_consumers(ptr addrspace(1) %base, i16 %count) {
; WIDE-LABEL: arbitrary_byte_consumers:
; WIDE:       ldp32
entry:
  br label %loop

loop:
  %p = phi ptr addrspace(1) [ %base, %entry ], [ %next, %loop ]
  %state = phi i16 [ 1, %entry ], [ %state.next, %loop ]
  %i = phi i16 [ 0, %entry ], [ %inc, %loop ]
  %value = load i8, ptr addrspace(1) %p, align 1
  %shift.count = and i8 %value, 7
  %shifted = lshr i8 %value, %shift.count
  %extended = zext i8 %shifted to i16
  %state.next = xor i16 %state, %extended
  %next = getelementptr i8, ptr addrspace(1) %p, i32 1
  %inc = add i16 %i, 1
  %done = icmp eq i16 %inc, %count
  br i1 %done, label %exit, label %loop

exit:
  %result = phi i16 [ %state.next, %loop ]
  ret i16 %result
}

define i16 @data_dependent_address(ptr addrspace(1) %base, i16 %count) {
; WIDE-LABEL: data_dependent_address:
; WIDE-NOT:   ldp16
; WIDE-NOT:   ldp32
; WIDE:       ldp8u
entry:
  br label %loop

loop:
  %p = phi ptr addrspace(1) [ %base, %entry ], [ %next, %loop ]
  %sum = phi i16 [ 0, %entry ], [ %sum.next, %loop ]
  %i = phi i16 [ 0, %entry ], [ %inc, %loop ]
  %value = load i8, ptr addrspace(1) %p, align 1
  %offset = zext i8 %value to i32
  %next = getelementptr i8, ptr addrspace(1) %base, i32 %offset
  %extended = zext i8 %value to i16
  %sum.next = add i16 %sum, %extended
  %inc = add i16 %i, 1
  %done = icmp eq i16 %inc, %count
  br i1 %done, label %exit, label %loop

exit:
  %result = phi i16 [ %sum.next, %loop ]
  ret i16 %result
}

define i16 @wrong_stride(ptr addrspace(1) %base, i16 %count) {
; WIDE-LABEL: wrong_stride:
; WIDE-NOT:   ldp16
; WIDE-NOT:   ldp32
; WIDE:       ldp8u
entry:
  br label %loop

loop:
  %p = phi ptr addrspace(1) [ %base, %entry ], [ %next, %loop ]
  %sum = phi i16 [ 0, %entry ], [ %sum.next, %loop ]
  %i = phi i16 [ 0, %entry ], [ %inc, %loop ]
  %value = load i8, ptr addrspace(1) %p, align 1
  %extended = zext i8 %value to i16
  %sum.next = add i16 %sum, %extended
  %next = getelementptr i8, ptr addrspace(1) %p, i32 2
  %inc = add i16 %i, 1
  %done = icmp eq i16 %inc, %count
  br i1 %done, label %exit, label %loop

exit:
  %result = phi i16 [ %sum.next, %loop ]
  ret i16 %result
}

define i16 @reverse_walk(ptr addrspace(1) %base, i16 %count) {
; WIDE-LABEL: reverse_walk:
; WIDE-NOT:   ldp16
; WIDE-NOT:   ldp32
; WIDE:       ldp8u
entry:
  br label %loop

loop:
  %p = phi ptr addrspace(1) [ %base, %entry ], [ %next, %loop ]
  %sum = phi i16 [ 0, %entry ], [ %sum.next, %loop ]
  %i = phi i16 [ 0, %entry ], [ %inc, %loop ]
  %value = load i8, ptr addrspace(1) %p, align 1
  %extended = zext i8 %value to i16
  %sum.next = add i16 %sum, %extended
  %next = getelementptr i8, ptr addrspace(1) %p, i32 -1
  %inc = add i16 %i, 1
  %done = icmp eq i16 %inc, %count
  br i1 %done, label %exit, label %loop

exit:
  %result = phi i16 [ %sum.next, %loop ]
  ret i16 %result
}

define i16 @two_program_streams(ptr addrspace(1) %a.base,
                                ptr addrspace(1) %b.base, i16 %count) {
; WIDE-LABEL: two_program_streams:
; WIDE-NOT:   ldp16
; WIDE-NOT:   ldp32
; WIDE:       ldp8u
; WIDE:       ldp8u
entry:
  br label %loop

loop:
  %a.p = phi ptr addrspace(1) [ %a.base, %entry ], [ %a.next, %loop ]
  %b.p = phi ptr addrspace(1) [ %b.base, %entry ], [ %b.next, %loop ]
  %sum = phi i16 [ 0, %entry ], [ %sum.next, %loop ]
  %i = phi i16 [ 0, %entry ], [ %inc, %loop ]
  %a.value = load i8, ptr addrspace(1) %a.p, align 1
  %b.value = load i8, ptr addrspace(1) %b.p, align 1
  %a.ext = zext i8 %a.value to i16
  %b.ext = zext i8 %b.value to i16
  %pair = add i16 %a.ext, %b.ext
  %sum.next = add i16 %sum, %pair
  %a.next = getelementptr i8, ptr addrspace(1) %a.p, i32 1
  %b.next = getelementptr i8, ptr addrspace(1) %b.p, i32 1
  %inc = add i16 %i, 1
  %done = icmp eq i16 %inc, %count
  br i1 %done, label %exit, label %loop

exit:
  %result = phi i16 [ %sum.next, %loop ]
  ret i16 %result
}

define i16 @volatile_adjacent_loads(ptr addrspace(1) %p) {
; WIDE-LABEL: volatile_adjacent_loads:
; WIDE-NOT:   ldp16
; WIDE-NOT:   ldp32
; WIDE:       ldp8u
; WIDE:       ldp8u
  %first = load volatile i8, ptr addrspace(1) %p, align 1
  %second.p = getelementptr i8, ptr addrspace(1) %p, i32 1
  %second = load volatile i8, ptr addrspace(1) %second.p, align 1
  %a = zext i8 %first to i16
  %b = zext i8 %second to i16
  %sum = add i16 %a, %b
  ret i16 %sum
}

define i16 @call_between_loads(ptr addrspace(1) %p) {
; WIDE-LABEL: call_between_loads:
; WIDE-NOT:   ldp16
; WIDE-NOT:   ldp32
; WIDE:       ldp8u
; WIDE:       ldp8u
  %first = load i8, ptr addrspace(1) %p, align 1
  call void @intervening_call()
  %second.p = getelementptr i8, ptr addrspace(1) %p, i32 1
  %second = load i8, ptr addrspace(1) %second.p, align 1
  %a = zext i8 %first to i16
  %b = zext i8 %second to i16
  %sum = add i16 %a, %b
  ret i16 %sum
}

define i16 @different_bases(ptr addrspace(1) %a, ptr addrspace(1) %b) {
; WIDE-LABEL: different_bases:
; WIDE-NOT:   ldp16
; WIDE-NOT:   ldp32
; WIDE:       ldp8u
; WIDE:       ldp8u
  %first = load i8, ptr addrspace(1) %a, align 1
  %second = load i8, ptr addrspace(1) %b, align 1
  %a.ext = zext i8 %first to i16
  %b.ext = zext i8 %second to i16
  %sum = add i16 %a.ext, %b.ext
  ret i16 %sum
}

define i16 @unroll_disabled(ptr addrspace(1) %base, i16 %count) {
; WIDE-LABEL: unroll_disabled:
; WIDE-NOT:   ldp16
; WIDE-NOT:   ldp32
; WIDE:       ldp8u {{.*}}[q{{[0-9]+}}+]
entry:
  br label %loop

loop:
  %p = phi ptr addrspace(1) [ %base, %entry ], [ %next, %loop ]
  %sum = phi i16 [ 0, %entry ], [ %sum.next, %loop ]
  %i = phi i16 [ 0, %entry ], [ %inc, %loop ]
  %value = load i8, ptr addrspace(1) %p, align 1
  %extended = zext i8 %value to i16
  %sum.next = add i16 %sum, %extended
  %next = getelementptr i8, ptr addrspace(1) %p, i32 1
  %inc = add i16 %i, 1
  %done = icmp eq i16 %inc, %count
  br i1 %done, label %exit, label %loop, !llvm.loop !0

exit:
  %result = phi i16 [ %sum.next, %loop ]
  ret i16 %result
}

define i16 @minsize_byte_stream(ptr addrspace(1) %base, i16 %count) minsize {
; WIDE-LABEL: minsize_byte_stream:
; WIDE-NOT:   ldp16
; WIDE-NOT:   ldp32
; WIDE:       ldp8u {{.*}}[q{{[0-9]+}}+]
entry:
  br label %loop

loop:
  %p = phi ptr addrspace(1) [ %base, %entry ], [ %next, %loop ]
  %sum = phi i16 [ 0, %entry ], [ %sum.next, %loop ]
  %i = phi i16 [ 0, %entry ], [ %inc, %loop ]
  %value = load i8, ptr addrspace(1) %p, align 1
  %extended = zext i8 %value to i16
  %sum.next = add i16 %sum, %extended
  %next = getelementptr i8, ptr addrspace(1) %p, i32 1
  %inc = add i16 %i, 1
  %done = icmp eq i16 %inc, %count
  br i1 %done, label %exit, label %loop

exit:
  %result = phi i16 [ %sum.next, %loop ]
  ret i16 %result
}

!0 = distinct !{!0, !1}
!1 = !{!"llvm.loop.unroll.disable"}
