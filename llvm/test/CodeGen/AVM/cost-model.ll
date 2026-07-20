; RUN: opt -mtriple=avm-unknown-arduboyfx -mcpu=avm1 \
; RUN:   -mtune=avm-interpreter-32u4-v1 -passes="print<cost-model>" \
; RUN:   -cost-kind=throughput -disable-output 2>&1 < %s | FileCheck %s

define void @scalar_costs(i16 %a, i16 %b, i16 %count) {
; CHECK-LABEL: Cost Model: Found an estimated cost of 13 for instruction: %udiv = udiv i16 %a, %b
; CHECK:       Cost Model: Found an estimated cost of 13 for instruction: %urem = urem i16 %a, %b
; CHECK:       Cost Model: Found an estimated cost of 13 for instruction: %sdiv = sdiv i16 %a, %b
; CHECK:       Cost Model: Found an estimated cost of 13 for instruction: %srem = srem i16 %a, %b
; CHECK:       Cost Model: Found an estimated cost of 1 for instruction: %shl1 = shl i16 %a, 1
; CHECK:       Cost Model: Found an estimated cost of 3 for instruction: %shl3 = shl i16 %a, 3
; CHECK:       Cost Model: Found an estimated cost of 4 for instruction: %shl4 = shl i16 %a, 4
; CHECK:       Cost Model: Found an estimated cost of 2 for instruction: %lshr1 = lshr i16 %a, 1
; CHECK:       Cost Model: Found an estimated cost of 3 for instruction: %lshr2 = lshr i16 %a, 2
; CHECK:       Cost Model: Found an estimated cost of 5 for instruction: %ashr15 = ashr i16 %a, 15
; CHECK:       Cost Model: Found an estimated cost of 4 for instruction: %variable = shl i16 %a, %count
  %udiv = udiv i16 %a, %b
  %urem = urem i16 %a, %b
  %sdiv = sdiv i16 %a, %b
  %srem = srem i16 %a, %b
  %shl1 = shl i16 %a, 1
  %shl3 = shl i16 %a, 3
  %shl4 = shl i16 %a, 4
  %lshr1 = lshr i16 %a, 1
  %lshr2 = lshr i16 %a, 2
  %ashr15 = ashr i16 %a, 15
  %variable = shl i16 %a, %count
  ret void
}

define void @wide_float_and_program_costs(i32 %a, i32 %b, float %x, float %y,
                                           ptr addrspace(1) %program) {
; CHECK-LABEL: Cost Model: Found an estimated cost of 2 for instruction: %add32 = add i32 %a, %b
; CHECK:       Cost Model: Found an estimated cost of 2 for instruction: %sub32 = sub i32 %a, %b
; CHECK:       Cost Model: Found an estimated cost of 2 for instruction: %and32 = and i32 %a, %b
; CHECK:       Cost Model: Found an estimated cost of 2 for instruction: %or32 = or i32 %a, %b
; CHECK:       Cost Model: Found an estimated cost of 2 for instruction: %xor32 = xor i32 %a, %b
; CHECK:       Cost Model: Found an estimated cost of 11 for instruction: %fadd = fadd float %x, %y
; CHECK:       Cost Model: Found an estimated cost of 11 for instruction: %fsub = fsub float %x, %y
; CHECK:       Cost Model: Found an estimated cost of 13 for instruction: %fmul = fmul float %x, %y
; CHECK:       Cost Model: Found an estimated cost of 33 for instruction: %fdiv = fdiv float %x, %y
; CHECK:       Cost Model: Found an estimated cost of 12 for instruction: %sitofp = sitofp i32 %a to float
; CHECK:       Cost Model: Found an estimated cost of 12 for instruction: %fptosi = fptosi float %x to i32
; CHECK:       Cost Model: Found an estimated cost of 20 for instruction: %load = load i32, ptr addrspace(1) %program, align 1
  %add32 = add i32 %a, %b
  %sub32 = sub i32 %a, %b
  %and32 = and i32 %a, %b
  %or32 = or i32 %a, %b
  %xor32 = xor i32 %a, %b
  %fadd = fadd float %x, %y
  %fsub = fsub float %x, %y
  %fmul = fmul float %x, %y
  %fdiv = fdiv float %x, %y
  %sitofp = sitofp i32 %a to float
  %fptosi = fptosi float %x to i32
  %load = load i32, ptr addrspace(1) %program, align 1
  ret void
}
