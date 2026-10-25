; RUN: llc -mtriple=avm -O2 -verify-machineinstrs -print-after=avm-aggregate-returns %s -o %t 2>&1 | FileCheck %s --check-prefix=IR
; RUN: FileCheck %s --check-prefix=ASM --implicit-check-not="sys memcpy" < %t

target datalayout = "e-p:16:8-p1:24:8-i8:8-i16:8-i32:8-i64:8-n8:16-S8-P1"
%sample = type { i32, i16, i16, i8 }

; Argument promotion can remove sret without eliminating the result pointer.
; The result need not be the first parameter; preserve remaining arguments and
; their attributes as register and stack assignments change.
define i32 @update(i16 %step, i32 %mask) {
; IR-LABEL: define i32 @update(
; IR: call addrspace(1) void @advance(i16 zeroext %step, ptr nonnull align 1 dereferenceable(9) %current, i32 %mask)
; IR-NOT: @llvm.memcpy
; IR: ret i32
; ASM-LABEL: update:
; ASM: call advance
; ASM: ret
  %current = alloca %sample, align 1
  %result = alloca %sample, align 1
  store %sample zeroinitializer, ptr %current, align 1
  call void @advance(i16 zeroext %step, ptr %result,
                     ptr byval(%sample) align 1 %current, i32 %mask)
  call void @llvm.memcpy.p0.p0.i16(ptr %current, ptr %result, i16 9, i1 false)
  %value = load i32, ptr %current, align 1
  ret i32 %value
}

define internal void @advance(i16 zeroext %step, ptr noalias writeonly %out,
                              ptr byval(%sample) align 1 %in, i32 %mask)
                              nounwind willreturn noinline {
; IR-LABEL: define internal void @advance(i16 zeroext %step, ptr nonnull align 1 dereferenceable(9) %in, i32 %mask)
; IR-NOT: @llvm.memcpy
; IR: ret void
; ASM-LABEL: advance:
; ASM: ret
  %value = load i32, ptr %in, align 1
  %wide = zext i16 %step to i32
  %sum = add i32 %value, %wide
  %next = xor i32 %sum, %mask
  store i32 %next, ptr %in, align 1
  call void @llvm.memcpy.p0.p0.i16(ptr %out, ptr %in, i16 9, i1 false)
  ret void
}

declare void @llvm.memcpy.p0.p0.i16(ptr, ptr, i16, i1 immarg)
