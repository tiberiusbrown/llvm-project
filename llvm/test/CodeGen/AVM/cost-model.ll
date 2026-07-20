; RUN: opt -mtriple=avm-unknown-arduboyfx -mcpu=avm1 \
; RUN:   -mtune=avm-interpreter-32u4-v1 -passes="print<cost-model>" \
; RUN:   -cost-kind=throughput -disable-output 2>&1 < %s | FileCheck %s
; RUN: opt -mtriple=avm-unknown-arduboyfx -mcpu=avm1 \
; RUN:   -mtune=avm-interpreter-32u4-v1 -passes="print<cost-model>" \
; RUN:   -cost-kind=code-size -disable-output 2>&1 < %s \
; RUN:   | FileCheck %s --check-prefix=SIZE

declare void @llvm.avm.debug.putc(i8)
declare void @llvm.avm.debug.break()
declare i16 @llvm.avm.millis()
declare i32 @llvm.avm.millis32()
declare float @llvm.avm.sinf(float)
declare float @llvm.avm.cosf(float)
declare float @llvm.avm.atan2f(float, float)
declare float @llvm.avm.tanf(float)
declare float @llvm.avm.expf(float)
declare float @llvm.avm.logf(float)
declare float @llvm.avm.log2f(float)
declare float @llvm.avm.log10f(float)
declare float @llvm.avm.powf(float, float)
declare float @llvm.avm.hypotf(float, float)
declare float @llvm.avm.fmodf(float, float)
declare i16 @llvm.bswap.i16(i16)
declare float @llvm.sqrt.f32(float)
declare float @llvm.fabs.f32(float)
declare float @llvm.minnum.f32(float, float)
declare float @llvm.maxnum.f32(float, float)
declare float @llvm.trunc.f32(float)
declare float @llvm.floor.f32(float)
declare float @llvm.ceil.f32(float)
declare float @llvm.round.f32(float)
declare i1 @llvm.is.fpclass.f32(float, i32 immarg)

define void @scalar_costs(i16 %a, i16 %b, i16 %count) {
; CHECK-LABEL: Cost Model: Found an estimated cost of 1 for instruction: %add = add i16 %a, %b
; CHECK:       Cost Model: Found an estimated cost of 1 for instruction: %sub = sub i16 %a, %b
; CHECK:       Cost Model: Found an estimated cost of 1 for instruction: %and = and i16 %a, %b
; CHECK:       Cost Model: Found an estimated cost of 1 for instruction: %or = or i16 %a, %b
; CHECK:       Cost Model: Found an estimated cost of 1 for instruction: %xor = xor i16 %a, %b
; CHECK:       Cost Model: Found an estimated cost of 3 for instruction: %mul = mul i16 %a, %b
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
  %add = add i16 %a, %b
  %sub = sub i16 %a, %b
  %and = and i16 %a, %b
  %or = or i16 %a, %b
  %xor = xor i16 %a, %b
  %mul = mul i16 %a, %b
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

define i16 @compare_select_and_memory(i16 %a, i16 %b, ptr %p,
                                      ptr %wide) {
; CHECK-LABEL: Cost Model: Found an estimated cost of 1 for instruction: %cmp = icmp ult i16 %a, %b
; CHECK:       Cost Model: Found an estimated cost of 2 for instruction: %select = select i1 %cmp, i16 %a, i16 %b
; CHECK:       Cost Model: Found an estimated cost of 1 for instruction: %byte = load i8, ptr %p, align 1
; CHECK:       Cost Model: Found an estimated cost of 1 for instruction: %word = load i16, ptr %p, align 1
; CHECK:       Cost Model: Found an estimated cost of 5 for instruction: %dword = load i32, ptr %wide, align 1
; CHECK:       Cost Model: Found an estimated cost of 5 for instruction: store i32 %dword, ptr %wide, align 1
  %cmp = icmp ult i16 %a, %b
  %select = select i1 %cmp, i16 %a, i16 %b
  %byte = load i8, ptr %p, align 1
  %word = load i16, ptr %p, align 1
  %dword = load i32, ptr %wide, align 1
  store i32 %dword, ptr %wide, align 1
  %extended = zext i8 %byte to i16
  %sum = add i16 %extended, %word
  %result = add i16 %sum, %select
  ret i16 %result
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

define void @remaining_float_costs(float %x, float %y) {
; CHECK-LABEL: Cost Model: Found an estimated cost of 3 for instruction: %neg = fneg float %x
; CHECK:       Cost Model: Found an estimated cost of 18 for instruction: %rem = frem float %x, %y
; CHECK:       Cost Model: Found an estimated cost of 33 for instruction: %sqrt = call{{.*}} float @llvm.sqrt.f32
; CHECK:       Cost Model: Found an estimated cost of 3 for instruction: %abs = call{{.*}} float @llvm.fabs.f32
; CHECK:       Cost Model: Found an estimated cost of 7 for instruction: %min = call{{.*}} float @llvm.minnum.f32
; CHECK:       Cost Model: Found an estimated cost of 7 for instruction: %max = call{{.*}} float @llvm.maxnum.f32
; CHECK:       Cost Model: Found an estimated cost of 7 for instruction: %trunc = call{{.*}} float @llvm.trunc.f32
; CHECK:       Cost Model: Found an estimated cost of 7 for instruction: %floor = call{{.*}} float @llvm.floor.f32
; CHECK:       Cost Model: Found an estimated cost of 7 for instruction: %ceil = call{{.*}} float @llvm.ceil.f32
; CHECK:       Cost Model: Found an estimated cost of 7 for instruction: %round = call{{.*}} float @llvm.round.f32
; CHECK:       Cost Model: Found an estimated cost of 8 for instruction: %class = call{{.*}} i1 @llvm.is.fpclass.f32
; SIZE-LABEL: Cost Model: Found an estimated cost of 2 for instruction: %neg = fneg float %x
; SIZE:       Cost Model: Found an estimated cost of 2 for instruction: %rem = frem float %x, %y
; SIZE:       Cost Model: Found an estimated cost of 2 for instruction: %sqrt = call{{.*}} float @llvm.sqrt.f32
; SIZE:       Cost Model: Found an estimated cost of 2 for instruction: %abs = call{{.*}} float @llvm.fabs.f32
; SIZE:       Cost Model: Found an estimated cost of 2 for instruction: %min = call{{.*}} float @llvm.minnum.f32
; SIZE:       Cost Model: Found an estimated cost of 2 for instruction: %max = call{{.*}} float @llvm.maxnum.f32
; SIZE:       Cost Model: Found an estimated cost of 2 for instruction: %trunc = call{{.*}} float @llvm.trunc.f32
; SIZE:       Cost Model: Found an estimated cost of 2 for instruction: %floor = call{{.*}} float @llvm.floor.f32
; SIZE:       Cost Model: Found an estimated cost of 2 for instruction: %ceil = call{{.*}} float @llvm.ceil.f32
; SIZE:       Cost Model: Found an estimated cost of 2 for instruction: %round = call{{.*}} float @llvm.round.f32
; SIZE:       Cost Model: Found an estimated cost of 3 for instruction: %class = call{{.*}} i1 @llvm.is.fpclass.f32
  %neg = fneg float %x
  %rem = frem float %x, %y
  %sqrt = call float @llvm.sqrt.f32(float %x)
  %abs = call float @llvm.fabs.f32(float %x)
  %min = call float @llvm.minnum.f32(float %x, float %y)
  %max = call float @llvm.maxnum.f32(float %x, float %y)
  %trunc = call float @llvm.trunc.f32(float %x)
  %floor = call float @llvm.floor.f32(float %x)
  %ceil = call float @llvm.ceil.f32(float %x)
  %round = call float @llvm.round.f32(float %x)
  %class = call i1 @llvm.is.fpclass.f32(float %x, i32 504)
  ret void
}

define void @service_costs(i8 %c, float %x, float %y) {
; CHECK-LABEL: Cost Model: Found an estimated cost of 2 for instruction: call{{.*}} void @llvm.avm.debug.putc
; CHECK:       Cost Model: Found an estimated cost of 2 for instruction: call{{.*}} void @llvm.avm.debug.break
; CHECK:       Cost Model: Found an estimated cost of 2 for instruction: %millis = call{{.*}} i16 @llvm.avm.millis
; CHECK:       Cost Model: Found an estimated cost of 2 for instruction: %millis32 = call{{.*}} i32 @llvm.avm.millis32
; CHECK:       Cost Model: Found an estimated cost of 106 for instruction: %sin = call{{.*}} float @llvm.avm.sinf
; CHECK:       Cost Model: Found an estimated cost of 106 for instruction: %cos = call{{.*}} float @llvm.avm.cosf
; CHECK:       Cost Model: Found an estimated cost of 171 for instruction: %atan = call{{.*}} float @llvm.avm.atan2f
; CHECK:       Cost Model: Found an estimated cost of 135 for instruction: %tan = call{{.*}} float @llvm.avm.tanf
; CHECK:       Cost Model: Found an estimated cost of 141 for instruction: %exp = call{{.*}} float @llvm.avm.expf
; CHECK:       Cost Model: Found an estimated cost of 129 for instruction: %log = call{{.*}} float @llvm.avm.logf
; CHECK:       Cost Model: Found an estimated cost of 124 for instruction: %log2 = call{{.*}} float @llvm.avm.log2f
; CHECK:       Cost Model: Found an estimated cost of 129 for instruction: %log10 = call{{.*}} float @llvm.avm.log10f
; CHECK:       Cost Model: Found an estimated cost of 271 for instruction: %pow = call{{.*}} float @llvm.avm.powf
; CHECK:       Cost Model: Found an estimated cost of 53 for instruction: %hypot = call{{.*}} float @llvm.avm.hypotf
; CHECK:       Cost Model: Found an estimated cost of 18 for instruction: %fmod = call{{.*}} float @llvm.avm.fmodf
; SIZE-LABEL: Cost Model: Found an estimated cost of 2 for instruction: call{{.*}} void @llvm.avm.debug.putc
; SIZE:       Cost Model: Found an estimated cost of 2 for instruction: %sin = call{{.*}} float @llvm.avm.sinf
; SIZE:       Cost Model: Found an estimated cost of 2 for instruction: %pow = call{{.*}} float @llvm.avm.powf
  call void @llvm.avm.debug.putc(i8 %c)
  call void @llvm.avm.debug.break()
  %millis = call i16 @llvm.avm.millis()
  %millis32 = call i32 @llvm.avm.millis32()
  %sin = call float @llvm.avm.sinf(float %x)
  %cos = call float @llvm.avm.cosf(float %x)
  %atan = call float @llvm.avm.atan2f(float %x, float %y)
  %tan = call float @llvm.avm.tanf(float %x)
  %exp = call float @llvm.avm.expf(float %x)
  %log = call float @llvm.avm.logf(float %x)
  %log2 = call float @llvm.avm.log2f(float %x)
  %log10 = call float @llvm.avm.log10f(float %x)
  %pow = call float @llvm.avm.powf(float %x, float %y)
  %hypot = call float @llvm.avm.hypotf(float %x, float %y)
  %fmod = call float @llvm.avm.fmodf(float %x, float %y)
  ret void
}

define void @helper_and_cast_costs(i32 %a, i32 %b, i64 %wide0, i64 %wide1,
                                   i8 %byte, i1 %condition) {
; CHECK-LABEL: Cost Model: Found an estimated cost of 15 for instruction: %mul32 = mul i32 %a, %b
; CHECK:       Cost Model: Found an estimated cost of 14 for instruction: %shift32 = shl i32 %a, %b
; CHECK:       Cost Model: Found an estimated cost of 21 for instruction: %mul64 = mul i64 %wide0, %wide1
; CHECK:       Cost Model: Found an estimated cost of 18 for instruction: %shift64 = shl i64 %wide0, %wide1
; CHECK:       Cost Model: Found an estimated cost of 2 for instruction: %zext = zext i8 %byte to i16
; CHECK:       Cost Model: Found an estimated cost of 2 for instruction: %sext = sext i8 %byte to i16
; CHECK:       Cost Model: Found an estimated cost of 2 for instruction: %bool = zext i1 %condition to i16
; SIZE-LABEL: Cost Model: Found an estimated cost of 10 for instruction: %mul32 = mul i32 %a, %b
; SIZE:       Cost Model: Found an estimated cost of 9 for instruction: %shift32 = shl i32 %a, %b
; SIZE:       Cost Model: Found an estimated cost of 16 for instruction: %mul64 = mul i64 %wide0, %wide1
; SIZE:       Cost Model: Found an estimated cost of 13 for instruction: %shift64 = shl i64 %wide0, %wide1
  %mul32 = mul i32 %a, %b
  %shift32 = shl i32 %a, %b
  %mul64 = mul i64 %wide0, %wide1
  %shift64 = shl i64 %wide0, %wide1
  %zext = zext i8 %byte to i16
  %sext = sext i8 %byte to i16
  %bool = zext i1 %condition to i16
  ret void
}

define void @normalization_and_misc_costs(ptr addrspace(1) %left,
                                          ptr addrspace(1) %right, i16 %word) {
; CHECK-LABEL: Cost Model: Found an estimated cost of 9 for instruction: %cmp = icmp ult ptr addrspace(1) %left, %right
; CHECK:       Cost Model: Found an estimated cost of 2 for instruction: %swap = call{{.*}} i16 @llvm.bswap.i16
; SIZE-LABEL: Cost Model: Found an estimated cost of 7 for instruction: %cmp = icmp ult ptr addrspace(1) %left, %right
; SIZE:       Cost Model: Found an estimated cost of 2 for instruction: %swap = call{{.*}} i16 @llvm.bswap.i16
  %cmp = icmp ult ptr addrspace(1) %left, %right
  %swap = call i16 @llvm.bswap.i16(i16 %word)
  ret void
}
