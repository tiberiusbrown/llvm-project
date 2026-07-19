; RUN: llc -mtriple=avm-unknown-arduboyfx -mcpu=avm1 \
; RUN:   -mtune=avm-interpreter-32u4-v1 -O2 -verify-machineinstrs < %s \
; RUN:   | FileCheck %s

@word = global i16 1, align 1
@bytes = global [4 x i8] c"abc\00", align 1
@record = global { i8, i16 } { i8 3, i16 1024 }, align 1

define i16 @load_word() {
; CHECK-LABEL: load_word:
; CHECK: ldm16 r4, [word]
  %value = load i16, ptr @word, align 1
  ret i16 %value
}

define void @store_record_word(i16 %value) {
; CHECK-LABEL: store_record_word:
; CHECK: stm16 [record+1], r4
  store i16 %value, ptr getelementptr ({ i8, i16 }, ptr @record, i16 0, i32 1), align 1
  ret void
}

define ptr @data_address() {
; CHECK-LABEL: data_address:
; CHECK: ldi16 r4, bytes
  ret ptr @bytes
}

define zeroext i8 @load_unsigned_char(ptr %ptr) {
; CHECK-LABEL: load_unsigned_char:
; CHECK: ld8u r4, [r4]
  %value = load i8, ptr %ptr, align 1
  ret i8 %value
}

define signext i8 @load_signed_char(ptr %ptr) {
; CHECK-LABEL: load_signed_char:
; CHECK: ld8u r4, [r4]
; CHECK-NEXT: sext8 r4
  %value = load i8, ptr %ptr, align 1
  ret i8 %value
}

define void @store_char(ptr %ptr, i8 %value) {
; CHECK-LABEL: store_char:
; CHECK: st8 [r4], r5
  store i8 %value, ptr %ptr, align 1
  ret void
}

define i16 @truncate_and_extend(i16 %value) {
; CHECK-LABEL: truncate_and_extend:
; CHECK: zext8 r4
  %byte = trunc i16 %value to i8
  %extended = zext i8 %byte to i16
  ret i16 %extended
}

define zeroext i8 @add_chars(i8 %left, i8 %right) {
; CHECK-LABEL: add_chars:
; CHECK: add r4, r5
; CHECK-NEXT: zext8 r4
  %sum = add i8 %left, %right
  ret i8 %sum
}

define i16 @unaligned_word(ptr %base) {
; CHECK-LABEL: unaligned_word:
; CHECK: ld16 {{r[0-7]}}, [{{r[0-7]}}]
  %ptr = getelementptr i8, ptr %base, i16 1
  %value = load i16, ptr %ptr, align 1
  ret i16 %value
}

define i16 @postincrement_word(ptr %ptr, ptr %updated) {
; CHECK-LABEL: postincrement_word:
; CHECK: ld16 [[VALUE:r[0-7]]], [r4+]
; CHECK: st16 [r5], r4
  %value = load i16, ptr %ptr, align 1
  %next = getelementptr i16, ptr %ptr, i16 1
  store ptr %next, ptr %updated, align 1
  ret i16 %value
}

define i16 @volatile_bytes(ptr %ptr) {
; CHECK-LABEL: volatile_bytes:
; CHECK: ld8u {{r[0-7]}}, [{{r[0-7]}}]
; CHECK: ld8u {{r[0-7]}}, [{{r[0-7]}}]
; CHECK-NOT: ld16
  %first = load volatile i8, ptr %ptr, align 1
  %next = getelementptr i8, ptr %ptr, i16 1
  %second = load volatile i8, ptr %next, align 1
  %a = zext i8 %first to i16
  %b = zext i8 %second to i16
  %sum = add i16 %a, %b
  ret i16 %sum
}

declare void @llvm.memcpy.p0.p0.i16(ptr, ptr, i16, i1 immarg)
declare void @llvm.memmove.p0.p0.i16(ptr, ptr, i16, i1 immarg)
declare void @llvm.memset.p0.i16(ptr, i8, i16, i1 immarg)

define void @small_memcpy(ptr %dst, ptr %src) {
; CHECK-LABEL: small_memcpy:
; CHECK-NOT: call
; CHECK: ret
  call void @llvm.memcpy.p0.p0.i16(ptr align 1 %dst, ptr align 1 %src, i16 4, i1 false)
  ret void
}

define void @small_memmove(ptr %dst, ptr %src) {
; CHECK-LABEL: small_memmove:
; CHECK-NOT: call
; CHECK: ret
  call void @llvm.memmove.p0.p0.i16(ptr align 1 %dst, ptr align 1 %src, i16 3, i1 false)
  ret void
}

define void @small_memset(ptr %dst, i8 %value) {
; CHECK-LABEL: small_memset:
; CHECK: st8
; CHECK-NOT: call
; CHECK: ret
  call void @llvm.memset.p0.i16(ptr align 1 %dst, i8 %value, i16 4, i1 false)
  ret void
}
