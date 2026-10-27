; RUN: llc -mtriple=avm -O2 -verify-machineinstrs < %s | FileCheck %s

declare float @llvm.trunc.f32(float)
declare float @llvm.floor.f32(float)
declare float @llvm.ceil.f32(float)
declare float @llvm.round.f32(float)

define float @trunc_value(float %x) {
; CHECK-LABEL: trunc_value:
; CHECK: ftrunc{{$}}
  %r = call float @llvm.trunc.f32(float %x)
  ret float %r
}

define float @floor_value(float %x) {
; CHECK-LABEL: floor_value:
; CHECK: ffloor{{$}}
  %r = call float @llvm.floor.f32(float %x)
  ret float %r
}

define float @ceil_value(float %x) {
; CHECK-LABEL: ceil_value:
; CHECK: fceil{{$}}
  %r = call float @llvm.ceil.f32(float %x)
  ret float %r
}

define float @round_value(float %x) {
; CHECK-LABEL: round_value:
; CHECK: fround{{$}}
  %r = call float @llvm.round.f32(float %x)
  ret float %r
}

define float @remainder(float %x, float %y) {
; CHECK-LABEL: remainder:
; CHECK: fmod{{$}}
  %r = frem float %x, %y
  ret float %r
}

define i32 @float_word(float %x) {
; CHECK-LABEL: float_word:
; CHECK: ret
  %word = bitcast float %x to i32
  ret i32 %word
}

define float @from_word(i32 %x) {
; CHECK-LABEL: from_word:
; CHECK: ret
  %value = bitcast i32 %x to float
  ret float %value
}
