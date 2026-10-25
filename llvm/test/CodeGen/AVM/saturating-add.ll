; RUN: llc -mtriple=avm -O2 -verify-machineinstrs < %s | FileCheck %s
; RUN: llc -mtriple=avm -O0 -verify-machineinstrs < %s | FileCheck %s --check-prefix=O0

declare i16 @llvm.uadd.sat.i16(i16, i16)
declare i32 @llvm.umin.i32(i32, i32)

define i16 @intrinsic(i16 %a, i16 %b) {
; CHECK-LABEL: intrinsic:
; CHECK-NOT: add32
; CHECK: add r{{[0-7]}}, r{{[0-7]}}
; CHECK-NOT: cmp32
; CHECK: cmp r{{[0-7]}}, r{{[0-7]}}
; CHECK: cmov.uge
; CHECK: ret
; O0-LABEL: intrinsic:
; O0-NOT: add32
; O0: add r{{[0-7]}}, r{{[0-7]}}
; O0-NOT: cmp32
; O0: cmp r{{[0-7]}}, r{{[0-7]}}
; O0: cmov.uge
; O0: ret
  %sat = call i16 @llvm.uadd.sat.i16(i16 %a, i16 %b)
  ret i16 %sat
}

; This is the widened-add-and-clamp idiom emitted by Clang. It needs only
; five instructions before the return, without constructing register pairs.
define i16 @widened(i16 %a, i16 %b) {
; CHECK-LABEL: widened:
; CHECK:      add [[SUM:r[0-7]]], [[OTHER:r[0-7]]]
; CHECK-NEXT: ldi16 [[MAX:r[0-7]]], 65535
; CHECK-NEXT: cmp [[SUM]], [[OTHER]]
; CHECK-NEXT: cmov.uge [[MAX]], [[SUM]]
; CHECK-NEXT: mov r4, [[MAX]]
; CHECK-NEXT: ret
  %left = zext i16 %a to i32
  %right = zext i16 %b to i32
  %sum = add nuw nsw i32 %left, %right
  %clamped = call i32 @llvm.umin.i32(i32 %sum, i32 65535)
  %result = trunc i32 %clamped to i16
  ret i16 %result
}

; A compare/select and a commuted min must reach the same lowering.
define i16 @widened_select(i16 %a, i16 %b) {
; CHECK-LABEL: widened_select:
; CHECK-NOT: add32
; CHECK: add r{{[0-7]}}, r{{[0-7]}}
; CHECK-NOT: cmp32
; CHECK: cmp r{{[0-7]}}, r{{[0-7]}}
; CHECK: cmov.uge
; CHECK: ret
  %left = zext i16 %a to i32
  %right = zext i16 %b to i32
  %sum = add i32 %right, %left
  %fits = icmp ule i32 %sum, 65535
  %clamped = select i1 %fits, i32 %sum, i32 65535
  %result = trunc i32 %clamped to i16
  ret i16 %result
}

; Preserve the zero high word when the caller uses the entire clamped value.
define i32 @wide_result(i16 %a, i16 %b) {
; CHECK-LABEL: wide_result:
; CHECK-NOT: add32
; CHECK: add r{{[0-7]}}, r{{[0-7]}}
; CHECK-NOT: cmp32
; CHECK: cmp r{{[0-7]}}, r{{[0-7]}}
; CHECK: cmov.uge
; CHECK: {{(xor|sub)}} r5, r5
; CHECK: ret
  %left = zext i16 %a to i32
  %right = zext i16 %b to i32
  %sum = add i32 %left, %right
  %clamped = call i32 @llvm.umin.i32(i32 65535, i32 %sum)
  ret i32 %clamped
}

; Range proofs cover masks and extending loads, not just zext instructions.
define i16 @masked(i32 %a, i32 %b) {
; CHECK-LABEL: masked:
; CHECK-NOT: add32
; CHECK: add r{{[0-7]}}, r{{[0-7]}}
; CHECK-NOT: cmp32
; CHECK: cmp r{{[0-7]}}, r{{[0-7]}}
; CHECK: cmov.uge
; CHECK: ret
  %left = and i32 %a, 65535
  %right = and i32 %b, 65535
  %sum = add i32 %left, %right
  %clamped = call i32 @llvm.umin.i32(i32 %sum, i32 65535)
  %result = trunc i32 %clamped to i16
  ret i16 %result
}

define i16 @loaded(ptr %a, ptr %b) {
; CHECK-LABEL: loaded:
; CHECK-NOT: add32
; CHECK: add r{{[0-7]}}, r{{[0-7]}}
; CHECK-NOT: cmp32
; CHECK: cmp r{{[0-7]}}, r{{[0-7]}}
; CHECK: cmov.uge
; CHECK: ret
  %x = load i16, ptr %a, align 1
  %y = load i16, ptr %b, align 1
  %left = zext i16 %x to i32
  %right = zext i16 %y to i32
  %sum = add i32 %left, %right
  %clamped = call i32 @llvm.umin.i32(i32 %sum, i32 65535)
  %result = trunc i32 %clamped to i16
  ret i16 %result
}

; A live full sum must not be truncated or duplicated just to narrow its clamp.
define i16 @shared_sum(i16 %a, i16 %b, ptr %out) {
; CHECK-LABEL: shared_sum:
; CHECK: add32
; CHECK: st32
; CHECK: cmp32
; CHECK: ret
  %left = zext i16 %a to i32
  %right = zext i16 %b to i32
  %sum = add i32 %left, %right
  store i32 %sum, ptr %out, align 1
  %clamped = call i32 @llvm.umin.i32(i32 %sum, i32 65535)
  %result = trunc i32 %clamped to i16
  ret i16 %result
}

; Neither an unrestricted wide operand nor a signed word has the required range.
define i16 @wide_operand(i32 %a, i16 %b) {
; CHECK-LABEL: wide_operand:
; CHECK: add32
; CHECK: cmp32
; CHECK: ret
  %right = zext i16 %b to i32
  %sum = add i32 %a, %right
  %clamped = call i32 @llvm.umin.i32(i32 %sum, i32 65535)
  %result = trunc i32 %clamped to i16
  ret i16 %result
}

define i16 @signed_operands(i16 %a, i16 %b) {
; CHECK-LABEL: signed_operands:
; CHECK: add32
; CHECK: cmp32
; CHECK: ret
  %left = sext i16 %a to i32
  %right = sext i16 %b to i32
  %sum = add i32 %left, %right
  %clamped = call i32 @llvm.umin.i32(i32 %sum, i32 65535)
  %result = trunc i32 %clamped to i16
  ret i16 %result
}

define i16 @different_limit(i16 %a, i16 %b) {
; CHECK-LABEL: different_limit:
; CHECK: add32
; CHECK: cmp32
; CHECK: ret
  %left = zext i16 %a to i32
  %right = zext i16 %b to i32
  %sum = add i32 %left, %right
  %clamped = call i32 @llvm.umin.i32(i32 %sum, i32 65534)
  %result = trunc i32 %clamped to i16
  ret i16 %result
}
