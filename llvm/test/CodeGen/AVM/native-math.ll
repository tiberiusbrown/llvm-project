; RUN: llc -mtriple=avm -O0 -stop-after=avm-fixed-float-regs %s -o - | FileCheck %s --check-prefix=FIXED
; RUN: llc -mtriple=avm -O0 -verify-machineinstrs %s -o - | FileCheck %s
; RUN: llc -mtriple=avm -O1 -verify-machineinstrs -filetype=obj %s -o %t.1
; RUN: llc -mtriple=avm -O2 -verify-machineinstrs %s -o - | FileCheck %s
; RUN: llc -mtriple=avm -O3 -verify-machineinstrs -filetype=obj %s -o %t.3
declare float @llvm.sqrt.f32(float)
define float @fsqrt(float %x) {
; CHECK-LABEL: fsqrt:
; CHECK-NOT: call
; CHECK: fsqrt{{$}}
  %r = call float @llvm.sqrt.f32(float %x)
  ret float %r
}
declare float @llvm.trunc.f32(float)
define float @ftrunc(float %x) {
; CHECK-LABEL: ftrunc:
; CHECK-NOT: call
; CHECK: ftrunc{{$}}
  %r = call float @llvm.trunc.f32(float %x)
  ret float %r
}
declare float @llvm.floor.f32(float)
define float @ffloor(float %x) {
; CHECK-LABEL: ffloor:
; CHECK-NOT: call
; CHECK: ffloor{{$}}
  %r = call float @llvm.floor.f32(float %x)
  ret float %r
}
declare float @llvm.ceil.f32(float)
define float @fceil(float %x) {
; CHECK-LABEL: fceil:
; CHECK-NOT: call
; CHECK: fceil{{$}}
  %r = call float @llvm.ceil.f32(float %x)
  ret float %r
}
declare float @llvm.round.f32(float)
define float @fround(float %x) {
; CHECK-LABEL: fround:
; CHECK-NOT: call
; CHECK: fround{{$}}
  %r = call float @llvm.round.f32(float %x)
  ret float %r
}
declare float @llvm.fma.f32(float, float, float)
declare {float, i16} @llvm.avm.rempio2(float)
define float @native_fma(float %a, float %b, float %c, ptr %saved) {
; CHECK-LABEL: native_fma:
; CHECK: fma{{$}}
  %r = call float @llvm.fma.f32(float %a, float %b, float %c)
  %live = fadd float %b, %c
  store float %live, ptr %saved, align 1
  ret float %r
}
define float @native_fmod(float %x, float %y) {
; CHECK-LABEL: native_fmod:
; CHECK: fmod{{$}}
  %r = frem float %x, %y
  ret float %r
}
define float @native_rempio2(float %x, float %live_q3, ptr %out) {
; FIXED: $r4r5, $r6 = FREMPIO2_PSEUDO $r4r5
; CHECK-LABEL: native_rempio2:
; CHECK: frempio2{{$}}
  %pair = call {float, i16} @llvm.avm.rempio2(float %x)
  %r = extractvalue {float, i16} %pair, 0
  %q = extractvalue {float, i16} %pair, 1
  store float %live_q3, ptr %out, align 1
  %odd = and i16 %q, 1
  %cond = icmp ne i16 %odd, 0
  %neg = fneg float %r
  %selected = select i1 %cond, float %neg, float %r
  %sum = fadd float %selected, %live_q3
  ret float %sum
}
