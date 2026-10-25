; RUN: llc -mtriple=avm -O2 -verify-machineinstrs -print-after=avm-aggregate-returns %s -o %t 2>&1 | FileCheck %s --check-prefix=IR
; RUN: FileCheck %s --check-prefix=ASM --implicit-check-not="sys memcpy" < %t
; RUN: llc -mtriple=avm -O0 -verify-machineinstrs %s -o - | FileCheck %s --check-prefix=COPY
; RUN: llc -mtriple=avm -O2 -avm-disable-aggregate-returns -verify-machineinstrs %s -o - | FileCheck %s --check-prefix=COPY

target datalayout = "e-p:16:8-p1:24:8-i8:8-i16:8-i32:8-i64:8-n8:16-S8-P1"
%sample = type { i32, i16, i16, i8 }

; Multiple callers and multiple returning paths share one specialized body.
; The unused result alloca and its lifetime markers must disappear as well.
define i32 @update(i16 %step) {
; IR-LABEL: define i32 @update(
; IR: %current = alloca %sample
; IR-NOT: alloca
; IR: call addrspace(1) void @advance(ptr nonnull align 1 dereferenceable(9) %current, i16 %step)
; IR-NOT: @llvm.memcpy
; IR-NOT: @llvm.lifetime
; IR: ret i32
; ASM-LABEL: update:
; ASM: call advance
; ASM: ret
; COPY-LABEL: update:
; COPY: sys memcpy
; COPY: call advance
; COPY: sys memcpy
; COPY: ret
  %current = alloca %sample, align 1
  %result = alloca %sample, align 1
  store %sample zeroinitializer, ptr %current, align 1
  call void @llvm.lifetime.start.p0(ptr %result)
  call void @advance(ptr sret(%sample) %result,
                     ptr byval(%sample) align 1 %current, i16 %step)
  call void @llvm.memcpy.p0.p0.i16(ptr %current, ptr %result, i16 9, i1 false)
  call void @llvm.lifetime.end.p0(ptr %result)
  %value = load i32, ptr %current, align 1
  ret i32 %value
}

define i8 @update_again(i16 %step) {
; IR-LABEL: define i8 @update_again(
; IR: call addrspace(1) void @advance(ptr nonnull align 1 dereferenceable(9) %current, i16 %step)
; IR-NOT: @llvm.memcpy
; IR: ret i8
; ASM-LABEL: update_again:
; ASM: call advance
; ASM: ret
  %current = alloca %sample, align 1
  %result = alloca %sample, align 1
  store %sample zeroinitializer, ptr %current, align 1
  call void @advance(ptr sret(%sample) %result,
                     ptr byval(%sample) align 1 %current, i16 %step)
  call void @llvm.memcpy.p0.p0.i16(ptr %current, ptr %result, i16 9, i1 false)
  %tag = getelementptr %sample, ptr %current, i16 0, i32 3
  %value = load i8, ptr %tag, align 1
  ret i8 %value
}

define internal void @advance(ptr noalias writeonly sret(%sample) %out,
                              ptr byval(%sample) align 1 %in, i16 %step)
                              nounwind willreturn noinline {
; IR-LABEL: define internal void @advance(ptr nonnull align 1 dereferenceable(9) %in, i16 %step)
; IR-NOT: @llvm.memcpy
; IR: ret void
; IR-NOT: @llvm.memcpy
; IR: ret void
; ASM-LABEL: advance:
; ASM: ret
; COPY-LABEL: advance:
; COPY: sys memcpy
  %p = getelementptr %sample, ptr %in, i16 0, i32 1
  %v = load i16, ptr %p, align 1
  %next = add i16 %v, %step
  store i16 %next, ptr %p, align 1
  %odd = icmp ne i16 %step, 0
  br i1 %odd, label %a, label %b
a:
  call void @llvm.memcpy.p0.p0.i16(ptr %out, ptr %in, i16 9, i1 false)
  ret void
b:
  %tag = getelementptr %sample, ptr %in, i16 0, i32 3
  store i8 42, ptr %tag, align 1
  call void @llvm.memcpy.p0.p0.i16(ptr %out, ptr %in, i16 9, i1 false)
  ret void
}

declare void @llvm.memcpy.p0.p0.i16(ptr, ptr, i16, i1 immarg)
declare void @llvm.lifetime.start.p0(ptr captures(none))
declare void @llvm.lifetime.end.p0(ptr captures(none))
