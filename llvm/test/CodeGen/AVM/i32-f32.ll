; RUN: llc -mtriple=avm-unknown-arduboyfx -mcpu=avm1 \
; RUN:   -mtune=avm-interpreter-32u4-v1 -O2 -verify-machineinstrs < %s \
; RUN:   | FileCheck %s

declare i32 @llvm.bswap.i32(i32)
declare float @llvm.fabs.f32(float)
declare float @llvm.sqrt.f32(float)
declare float @llvm.minnum.f32(float, float)
declare float @llvm.maxnum.f32(float, float)
declare i1 @llvm.is.fpclass.f32(float, i32 immarg)
declare float @llvm.experimental.constrained.fadd.f32(float, float, metadata,
                                                      metadata)
declare float @llvm.experimental.constrained.fsub.f32(float, float, metadata,
                                                      metadata)
declare float @llvm.experimental.constrained.fmul.f32(float, float, metadata,
                                                      metadata)
declare float @llvm.experimental.constrained.fdiv.f32(float, float, metadata,
                                                      metadata)
declare float @llvm.experimental.constrained.sqrt.f32(float, metadata,
                                                      metadata)
declare float @llvm.experimental.constrained.sitofp.f32.i32(i32, metadata,
                                                            metadata)
declare float @llvm.experimental.constrained.uitofp.f32.i32(i32, metadata,
                                                            metadata)
declare i32 @llvm.experimental.constrained.fptosi.i32.f32(float, metadata)
declare i32 @llvm.experimental.constrained.fptoui.i32.f32(float, metadata)

define i32 @integer_arithmetic(i32 %a, i32 %b) {
; CHECK-LABEL: integer_arithmetic:
; CHECK:       add32
; CHECK:       and
; CHECK:       or
; CHECK:       xor
  %add = add i32 %a, %b
  %sub = sub i32 %add, 305419896
  %and = and i32 %sub, %a
  %or = or i32 %and, %b
  %xor = xor i32 %or, -1
  ret i32 %xor
}

define i1 @integer_compare(i32 %a, i32 %b) {
; CHECK-LABEL: integer_compare:
; CHECK:       cmp32
; CHECK:       cset
  %cmp = icmp ult i32 %a, %b
  ret i1 %cmp
}

define i32 @integer_subtract(i32 %a, i32 %b) {
; CHECK-LABEL: integer_subtract:
; CHECK:       sub32
  %result = sub i32 %a, %b
  ret i32 %result
}

define i32 @integer_bswap(i32 %value) {
; CHECK-LABEL: integer_bswap:
; CHECK:       bswap16
; CHECK:       bswap16
  %result = call i32 @llvm.bswap.i32(i32 %value)
  ret i32 %result
}

define i32 @integer_helpers(i32 %a, i32 %b, i16 %count) {
; CHECK-LABEL: integer_helpers:
; CHECK:       call __avm_mulsi3
; CHECK:       call __avm_udivsi3
; CHECK:       call __avm_lshrsi3
  %mul = mul i32 %a, %b
  %div = udiv i32 %mul, %b
  %count32 = zext i16 %count to i32
  %shift = lshr i32 %div, %count32
  ret i32 %shift
}

define i32 @signed_division_helper(i32 %a, i32 %b) {
; CHECK-LABEL: signed_division_helper:
; CHECK:       jmp __avm_divsi3
  %result = sdiv i32 %a, %b
  ret i32 %result
}

define i32 @signed_remainder_helper(i32 %a, i32 %b) {
; CHECK-LABEL: signed_remainder_helper:
; CHECK:       jmp __avm_modsi3
  %result = srem i32 %a, %b
  ret i32 %result
}

define i32 @unsigned_remainder_helper(i32 %a, i32 %b) {
; CHECK-LABEL: unsigned_remainder_helper:
; CHECK:       jmp __avm_umodsi3
  %result = urem i32 %a, %b
  ret i32 %result
}

define i32 @left_shift_helper(i32 %value, i32 %count) {
; CHECK-LABEL: left_shift_helper:
; CHECK:       jmp __avm_ashlsi3
  %result = shl i32 %value, %count
  ret i32 %result
}

define i32 @arithmetic_shift_helper(i32 %value, i32 %count) {
; CHECK-LABEL: arithmetic_shift_helper:
; CHECK:       jmp __avm_ashrsi3
  %result = ashr i32 %value, %count
  ret i32 %result
}

define i32 @load_store_i32(ptr %source, ptr %destination) {
; CHECK-LABEL: load_store_i32:
; CHECK:       ld32
; CHECK:       st32
  %value = load i32, ptr %source, align 1
  store i32 %value, ptr %destination, align 1
  ret i32 %value
}

define float @float_arithmetic(float %a, float %b) {
; CHECK-LABEL: float_arithmetic:
; CHECK:       fadd
; CHECK:       fsub
; CHECK:       fmul
; CHECK:       fdiv
; CHECK:       fabs
; CHECK:       fsqrt
; CHECK:       fmin
; CHECK:       fmax
  %add = fadd float %a, %b
  %sub = fsub float %add, %a
  %mul = fmul float %sub, %b
  %div = fdiv float %mul, %a
  %neg = fneg float %div
  %abs = call float @llvm.fabs.f32(float %neg)
  %sqrt = call float @llvm.sqrt.f32(float %abs)
  %min = call float @llvm.minnum.f32(float %sqrt, float %a)
  %max = call float @llvm.maxnum.f32(float %min, float %b)
  ret float %max
}

define float @float_negate(float %value) {
; CHECK-LABEL: float_negate:
; CHECK:       fneg
  %result = fneg float %value
  ret float %result
}

define float @integer_to_float(i32 %s, i32 %u) {
; CHECK-LABEL: integer_to_float:
; CHECK-DAG:   s32tof
; CHECK-DAG:   u32tof
; CHECK:       fadd
  %sf = sitofp i32 %s to float
  %uf = uitofp i32 %u to float
  %sum = fadd float %sf, %uf
  ret float %sum
}

define i32 @float_to_integer(float %value) {
; CHECK-LABEL: float_to_integer:
; CHECK-DAG:   ftos32
; CHECK-DAG:   ftou32
; CHECK:       add32
  %s = fptosi float %value to i32
  %u = fptoui float %value to i32
  %sum = add i32 %s, %u
  ret i32 %sum
}

define float @narrow_integer_to_float(i16 %s, i16 %u) {
; CHECK-LABEL: narrow_integer_to_float:
; CHECK-DAG:   s16tof
; CHECK-DAG:   u16tof
  %sf = sitofp i16 %s to float
  %uf = uitofp i16 %u to float
  %sum = fadd float %sf, %uf
  ret float %sum
}

define i16 @float_to_narrow_integer(float %value) {
; CHECK-LABEL: float_to_narrow_integer:
; CHECK-DAG:   ftos16
; CHECK-DAG:   ftou16
  %s = fptosi float %value to i16
  %u = fptoui float %value to i16
  %sum = add i16 %s, %u
  ret i16 %sum
}

define i1 @float_ordered_compare(float %a, float %b) {
; CHECK-LABEL: float_ordered_compare:
; CHECK:       fcmp
  %cmp = fcmp olt float %a, %b
  ret i1 %cmp
}

define i1 @float_unordered_compare(float %a, float %b) {
; CHECK-LABEL: float_unordered_compare:
; CHECK:       fcmp
  %cmp = fcmp une float %a, %b
  ret i1 %cmp
}

define i1 @float_is_nan(float %value) {
; CHECK-LABEL: float_is_nan:
; CHECK:       fclass
  %result = call i1 @llvm.is.fpclass.f32(float %value, i32 3)
  ret i1 %result
}

define i1 @float_oeq(float %a, float %b) {
; CHECK-LABEL: float_oeq:
; CHECK:       fcmp
  %result = fcmp oeq float %a, %b
  ret i1 %result
}

define i1 @float_ogt(float %a, float %b) {
; CHECK-LABEL: float_ogt:
; CHECK:       fcmp
  %result = fcmp ogt float %a, %b
  ret i1 %result
}

define i1 @float_oge(float %a, float %b) {
; CHECK-LABEL: float_oge:
; CHECK:       fcmp
  %result = fcmp oge float %a, %b
  ret i1 %result
}

define i1 @float_ole(float %a, float %b) {
; CHECK-LABEL: float_ole:
; CHECK:       fcmp
  %result = fcmp ole float %a, %b
  ret i1 %result
}

define i1 @float_one(float %a, float %b) {
; CHECK-LABEL: float_one:
; CHECK:       fcmp
  %result = fcmp one float %a, %b
  ret i1 %result
}

define i1 @float_ord(float %a, float %b) {
; CHECK-LABEL: float_ord:
; CHECK:       fcmp
  %result = fcmp ord float %a, %b
  ret i1 %result
}

define i1 @float_uno(float %a, float %b) {
; CHECK-LABEL: float_uno:
; CHECK:       fcmp
  %result = fcmp uno float %a, %b
  ret i1 %result
}

define i1 @float_ueq(float %a, float %b) {
; CHECK-LABEL: float_ueq:
; CHECK:       fcmp
  %result = fcmp ueq float %a, %b
  ret i1 %result
}

define i1 @float_ugt(float %a, float %b) {
; CHECK-LABEL: float_ugt:
; CHECK:       fcmp
  %result = fcmp ugt float %a, %b
  ret i1 %result
}

define i1 @float_uge(float %a, float %b) {
; CHECK-LABEL: float_uge:
; CHECK:       fcmp
  %result = fcmp uge float %a, %b
  ret i1 %result
}

define i1 @float_ult(float %a, float %b) {
; CHECK-LABEL: float_ult:
; CHECK:       fcmp
  %result = fcmp ult float %a, %b
  ret i1 %result
}

define i1 @float_ule(float %a, float %b) {
; CHECK-LABEL: float_ule:
; CHECK:       fcmp
  %result = fcmp ule float %a, %b
  ret i1 %result
}

define float @strict_float_add(float %a, float %b) strictfp {
; CHECK-LABEL: strict_float_add:
; CHECK:       jmp __addsf3
  %result = call float @llvm.experimental.constrained.fadd.f32(
      float %a, float %b, metadata !"round.dynamic",
      metadata !"fpexcept.strict")
  ret float %result
}

define float @strict_float_arithmetic(float %a, float %b) strictfp {
; CHECK-LABEL: strict_float_arithmetic:
; CHECK:       call __subsf3
; CHECK:       call __mulsf3
; CHECK:       call __divsf3
; CHECK:       call sqrtf
  %sub = call float @llvm.experimental.constrained.fsub.f32(
      float %a, float %b, metadata !"round.dynamic",
      metadata !"fpexcept.strict")
  %mul = call float @llvm.experimental.constrained.fmul.f32(
      float %sub, float %b, metadata !"round.dynamic",
      metadata !"fpexcept.strict")
  %div = call float @llvm.experimental.constrained.fdiv.f32(
      float %mul, float %a, metadata !"round.dynamic",
      metadata !"fpexcept.strict")
  %result = call float @llvm.experimental.constrained.sqrt.f32(
      float %div, metadata !"round.dynamic", metadata !"fpexcept.strict")
  ret float %result
}

define float @strict_integer_to_float(i32 %s, i32 %u) strictfp {
; CHECK-LABEL: strict_integer_to_float:
; CHECK:       call __floatsisf
; CHECK:       call __floatsisf
  %sf = call float @llvm.experimental.constrained.sitofp.f32.i32(
      i32 %s, metadata !"round.dynamic", metadata !"fpexcept.strict")
  %uf = call float @llvm.experimental.constrained.uitofp.f32.i32(
      i32 %u, metadata !"round.dynamic", metadata !"fpexcept.strict")
  %result = call float @llvm.experimental.constrained.fadd.f32(
      float %sf, float %uf, metadata !"round.dynamic",
      metadata !"fpexcept.strict")
  ret float %result
}

define i32 @strict_float_to_integer(float %value) strictfp {
; CHECK-LABEL: strict_float_to_integer:
; CHECK:       call __fixsfsi
; CHECK:       call __fixunssfsi
  %s = call i32 @llvm.experimental.constrained.fptosi.i32.f32(
      float %value, metadata !"fpexcept.strict")
  %u = call i32 @llvm.experimental.constrained.fptoui.i32.f32(
      float %value, metadata !"fpexcept.strict")
  %result = add i32 %s, %u
  ret i32 %result
}
