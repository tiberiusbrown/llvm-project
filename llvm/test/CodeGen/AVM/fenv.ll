; RUN: llc -mtriple=avm -O0 -verify-machineinstrs %s -o - | FileCheck %s
; RUN: llc -mtriple=avm -O2 -verify-machineinstrs %s -o - | FileCheck %s
; The invariant environment must use native operations without libcalls.
declare float @llvm.experimental.constrained.fadd.f32(float, float, metadata, metadata)
define float @strict_fadd(float %a, float %b) strictfp {
; CHECK-LABEL: strict_fadd:
; CHECK-NOT: call
; CHECK: fadd{{$}}
  %r = call float @llvm.experimental.constrained.fadd.f32(float %a, float %b, metadata !"round.dynamic", metadata !"fpexcept.strict")
  ret float %r
}
declare float @llvm.experimental.constrained.fsub.f32(float, float, metadata, metadata)
define float @strict_fsub(float %a, float %b) strictfp {
; CHECK-LABEL: strict_fsub:
; CHECK-NOT: call
; CHECK: fsub{{$}}
  %r = call float @llvm.experimental.constrained.fsub.f32(float %a, float %b, metadata !"round.dynamic", metadata !"fpexcept.strict")
  ret float %r
}
declare float @llvm.experimental.constrained.fmul.f32(float, float, metadata, metadata)
define float @strict_fmul(float %a, float %b) strictfp {
; CHECK-LABEL: strict_fmul:
; CHECK-NOT: call
; CHECK: fmul{{$}}
  %r = call float @llvm.experimental.constrained.fmul.f32(float %a, float %b, metadata !"round.dynamic", metadata !"fpexcept.strict")
  ret float %r
}
declare float @llvm.experimental.constrained.fdiv.f32(float, float, metadata, metadata)
define float @strict_fdiv(float %a, float %b) strictfp {
; CHECK-LABEL: strict_fdiv:
; CHECK-NOT: call
; CHECK: fdiv{{$}}
  %r = call float @llvm.experimental.constrained.fdiv.f32(float %a, float %b, metadata !"round.dynamic", metadata !"fpexcept.strict")
  ret float %r
}
declare float @llvm.experimental.constrained.sqrt.f32(float, metadata, metadata)
define float @strict_sqrt(float %a) strictfp {
; CHECK-LABEL: strict_sqrt:
; CHECK-NOT: call
; CHECK: fsqrt{{$}}
  %r = call float @llvm.experimental.constrained.sqrt.f32(float %a, metadata !"round.tonearest", metadata !"fpexcept.strict")
  ret float %r
}
declare i1 @llvm.experimental.constrained.fcmp.f32(float, float, metadata, metadata)
define i1 @strict_compare(float %a, float %b) strictfp {
; CHECK-LABEL: strict_compare:
; CHECK-NOT: call
; CHECK: fcmp{{$}}
  %r = call i1 @llvm.experimental.constrained.fcmp.f32(float %a, float %b, metadata !"olt", metadata !"fpexcept.strict")
  ret i1 %r
}
declare float @llvm.experimental.constrained.sitofp.f32.i16(i16, metadata, metadata)
define float @strict_sitofp16(i16 %a) strictfp {
; CHECK-LABEL: strict_sitofp16:
; CHECK-NOT: call
; CHECK: s16tof{{$}}
  %r = call float @llvm.experimental.constrained.sitofp.f32.i16(i16 %a, metadata !"round.dynamic", metadata !"fpexcept.strict")
  ret float %r
}
declare i16 @llvm.experimental.constrained.fptosi.i16.f32(float, metadata)
define i16 @strict_fptosi16(float %a) strictfp {
; CHECK-LABEL: strict_fptosi16:
; CHECK-NOT: call
; CHECK: ftos16{{$}}
  %r = call i16 @llvm.experimental.constrained.fptosi.i16.f32(float %a, metadata !"fpexcept.strict")
  ret i16 %r
}
declare float @llvm.experimental.constrained.uitofp.f32.i16(i16, metadata, metadata)
define float @strict_uitofp16(i16 %a) strictfp {
; CHECK-LABEL: strict_uitofp16:
; CHECK-NOT: call
; CHECK: u16tof{{$}}
  %r = call float @llvm.experimental.constrained.uitofp.f32.i16(i16 %a, metadata !"round.dynamic", metadata !"fpexcept.strict")
  ret float %r
}
declare i16 @llvm.experimental.constrained.fptoui.i16.f32(float, metadata)
define i16 @strict_fptoui16(float %a) strictfp {
; CHECK-LABEL: strict_fptoui16:
; CHECK-NOT: call
; CHECK: ftou16{{$}}
  %r = call i16 @llvm.experimental.constrained.fptoui.i16.f32(float %a, metadata !"fpexcept.strict")
  ret i16 %r
}
declare float @llvm.experimental.constrained.sitofp.f32.i32(i32, metadata, metadata)
define float @strict_sitofp32(i32 %a) strictfp {
; CHECK-LABEL: strict_sitofp32:
; CHECK-NOT: call
; CHECK: s32tof{{$}}
  %r = call float @llvm.experimental.constrained.sitofp.f32.i32(i32 %a, metadata !"round.dynamic", metadata !"fpexcept.strict")
  ret float %r
}
declare i32 @llvm.experimental.constrained.fptosi.i32.f32(float, metadata)
define i32 @strict_fptosi32(float %a) strictfp {
; CHECK-LABEL: strict_fptosi32:
; CHECK-NOT: call
; CHECK: ftos32{{$}}
  %r = call i32 @llvm.experimental.constrained.fptosi.i32.f32(float %a, metadata !"fpexcept.strict")
  ret i32 %r
}
declare float @llvm.experimental.constrained.uitofp.f32.i32(i32, metadata, metadata)
define float @strict_uitofp32(i32 %a) strictfp {
; CHECK-LABEL: strict_uitofp32:
; CHECK-NOT: call
; CHECK: u32tof{{$}}
  %r = call float @llvm.experimental.constrained.uitofp.f32.i32(i32 %a, metadata !"round.dynamic", metadata !"fpexcept.strict")
  ret float %r
}
declare i32 @llvm.experimental.constrained.fptoui.i32.f32(float, metadata)
define i32 @strict_fptoui32(float %a) strictfp {
; CHECK-LABEL: strict_fptoui32:
; CHECK-NOT: call
; CHECK: ftou32{{$}}
  %r = call i32 @llvm.experimental.constrained.fptoui.i32.f32(float %a, metadata !"fpexcept.strict")
  ret i32 %r
}

declare i1 @llvm.experimental.constrained.fcmps.f32(float, float, metadata, metadata)
define i1 @strict_signaling_compare(float %a, float %b) strictfp {
; CHECK-LABEL: strict_signaling_compare:
; CHECK-NOT: call
; CHECK: fcmp{{$}}
  %r = call i1 @llvm.experimental.constrained.fcmps.f32(float %a, float %b, metadata !"olt", metadata !"fpexcept.strict")
  ret i1 %r
}
