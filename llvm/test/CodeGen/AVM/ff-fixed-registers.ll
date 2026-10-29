; RUN: llc -mtriple=avm -O0 -verify-machineinstrs -filetype=obj %s -o %t.0
; RUN: llc -mtriple=avm -O1 -verify-machineinstrs -filetype=obj %s -o %t.1
; RUN: llc -mtriple=avm -O2 -verify-machineinstrs -filetype=obj %s -o %t.2
; RUN: llc -mtriple=avm -O3 -verify-machineinstrs -filetype=obj %s -o %t.3
; RUN: llc -mtriple=avm -O2 -verify-machineinstrs %s -o - | FileCheck %s

declare i1 @llvm.is.fpclass.f32(float, i32 immarg)

define float @live_arithmetic(float %a, float %b) {
; CHECK-LABEL: live_arithmetic:
; CHECK-DAG: fadd{{$}}
; CHECK-DAG: fsub{{$}}
; CHECK: fmul{{$}}
; CHECK: fdiv{{$}}
  %add = fadd float %a, %b
  %sub = fsub float %a, %b
  %mul = fmul float %add, %sub
  %div = fdiv float %mul, %b
  %r = fadd float %div, %a
  ret float %r
}
define float @live_cmp_class(float %a, float %b) {
; CHECK-LABEL: live_cmp_class:
; CHECK-DAG: fcmp{{$}}
  %cmp = fcmp ult float %a, %b
  %class = call i1 @llvm.is.fpclass.f32(float %a, i32 256)
  %choose = xor i1 %cmp, %class
  %r = select i1 %choose, float %a, float %b
  ret float %r
}
define i32 @conversions(i16 %s, i16 %u, i32 %l, i32 %ul, float %f) {
; CHECK-LABEL: conversions:
; CHECK-DAG: s16tof{{$}}
; CHECK-DAG: u16tof{{$}}
; CHECK-DAG: s32tof{{$}}
; CHECK-DAG: u32tof{{$}}
; CHECK-DAG: ftos16{{$}}
; CHECK-DAG: ftou16{{$}}
; CHECK-DAG: ftos32{{$}}
; CHECK-DAG: ftou32{{$}}
  %a = sitofp i16 %s to float
  %b = uitofp i16 %u to float
  %c = sitofp i32 %l to float
  %d = uitofp i32 %ul to float
  %x = fadd float %a, %b
  %y = fadd float %c, %d
  %z = fsub float %x, %y
  %v = fadd float %z, %f
  %si = fptosi float %v to i16
  %ui = fptoui float %v to i16
  %li = fptosi float %v to i32
  %uli = fptoui float %v to i32
  %sx = sext i16 %si to i32
  %ux = zext i16 %ui to i32
  %r1 = add i32 %sx, %ux
  %r2 = xor i32 %li, %uli
  %r = add i32 %r1, %r2
  ret i32 %r
}
