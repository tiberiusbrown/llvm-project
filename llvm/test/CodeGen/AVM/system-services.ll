; RUN: llc -mtriple=avm -O0 -verify-machineinstrs < %s | FileCheck %s
; RUN: llc -mtriple=avm -O2 -verify-machineinstrs < %s | FileCheck %s
; RUN: llc -mtriple=avm -O0 -stop-after=avm-service-result < %s -o - \
; RUN:   | FileCheck %s --check-prefix=MIR
; RUN: llc -mtriple=avm -O2 -stop-after=avm-service-result < %s -o - \
; RUN:   | FileCheck %s --check-prefix=MIR

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

define void @debug_services(i8 %value) {
; CHECK-LABEL: debug_services:
; CHECK:       sys debug_putc
; CHECK-NEXT:  sys debug_break
  call void @llvm.avm.debug.putc(i8 %value)
  call void @llvm.avm.debug.break()
  ret void
}

define void @timer_result_to_debug() {
; CHECK-LABEL: timer_result_to_debug:
; CHECK:       sys millis
; CHECK:       sys debug_putc
  %timer = call i16 @llvm.avm.millis()
  %character = trunc i16 %timer to i8
  call void @llvm.avm.debug.putc(i8 %character)
  ret void
}

define i16 @timer_services() {
; CHECK-LABEL: timer_services:
; CHECK:       sys millis
; CHECK:       sys millis
  %unused = call i16 @llvm.avm.millis()
  %result = call i16 @llvm.avm.millis()
  ret i16 %result
}

define i32 @timer32_service() {
; CHECK-LABEL: timer32_service:
; CHECK:       sys millis32
  %result = call i32 @llvm.avm.millis32()
  ret i32 %result
}

define float @math_services(float %x, float %y) {
; CHECK-LABEL: math_services:
; CHECK:       sys sinf
; CHECK:       sys cosf
; CHECK:       sys atan2f
; CHECK:       sys tanf
; CHECK:       sys expf
; CHECK:       sys logf
; CHECK:       sys log2f
; CHECK:       sys log10f
; CHECK:       sys powf
; CHECK:       sys hypotf
; CHECK:       sys fmodf
  %sin = call float @llvm.avm.sinf(float %x)
  %cos = call float @llvm.avm.cosf(float %sin)
  %atan = call float @llvm.avm.atan2f(float %cos, float %y)
  %tan = call float @llvm.avm.tanf(float %atan)
  %exp = call float @llvm.avm.expf(float %tan)
  %log = call float @llvm.avm.logf(float %exp)
  %log2 = call float @llvm.avm.log2f(float %log)
  %log10 = call float @llvm.avm.log10f(float %log2)
  %pow = call float @llvm.avm.powf(float %log10, float %y)
  %hypot = call float @llvm.avm.hypotf(float %pow, float %x)
  %fmod = call float @llvm.avm.fmodf(float %hypot, float %y)
  ret float %fmod
}

; MIR-LABEL: name: debug_services
; MIR:       SYS_DEBUG_PUTC_PSEUDO
; MIR:       SYS_DEBUG_BREAK_PSEUDO
; MIR-LABEL: name: timer_result_to_debug
; MIR:       [[DEBUG_RESULT:%[0-9]+]]:r4only = SYS_MILLIS_PSEUDO
; MIR-NEXT:  [[DEBUG_GENERAL:%[0-9]+]]:gpr16 = nomerge COPY killed [[DEBUG_RESULT]]
; MIR:       [[DEBUG_INPUT:%[0-9]+]]:r4only = COPY {{%[0-9]+}}
; MIR-NEXT:  SYS_DEBUG_PUTC_PSEUDO killed [[DEBUG_INPUT]]
; MIR-LABEL: name: timer_services
; MIR:       {{%[0-9]+}}:r4only = SYS_MILLIS_PSEUDO
; MIR-NEXT:  [[TIMER:%[0-9]+]]:r4only = SYS_MILLIS_PSEUDO
; MIR-NEXT:  {{%[0-9]+}}:gpr16 = nomerge COPY{{.*}} [[TIMER]]
; MIR-LABEL: name: timer32_service
; MIR:       [[TIMER32:%[0-9]+]]:q2only = SYS_MILLIS32_PSEUDO
; MIR-NEXT:  {{%[0-9]+}}:gpr32 = nomerge COPY{{.*}} [[TIMER32]]
; MIR-LABEL: name: math_services
; MIR:       [[SINF:%[0-9]+]]:q2only = SYS_SINF_PSEUDO
; MIR-NEXT:  {{%[0-9]+}}:gpr32 = nomerge COPY{{.*}} [[SINF]]
; MIR:       [[COSF:%[0-9]+]]:q2only = SYS_COSF_PSEUDO
; MIR-NEXT:  {{%[0-9]+}}:gpr32 = nomerge COPY{{.*}} [[COSF]]
; MIR:       [[ATAN2F:%[0-9]+]]:q2only = SYS_ATAN2F_PSEUDO {{%[0-9]+}}, {{%[0-9]+}}
; MIR-NEXT:  {{%[0-9]+}}:gpr32 = nomerge COPY{{.*}} [[ATAN2F]]
; MIR:       [[TANF:%[0-9]+]]:q2only = SYS_TANF_PSEUDO
; MIR-NEXT:  {{%[0-9]+}}:gpr32 = nomerge COPY{{.*}} [[TANF]]
; MIR:       [[EXPF:%[0-9]+]]:q2only = SYS_EXPF_PSEUDO
; MIR-NEXT:  {{%[0-9]+}}:gpr32 = nomerge COPY{{.*}} [[EXPF]]
; MIR:       [[LOGF:%[0-9]+]]:q2only = SYS_LOGF_PSEUDO
; MIR-NEXT:  {{%[0-9]+}}:gpr32 = nomerge COPY{{.*}} [[LOGF]]
; MIR:       [[LOG2F:%[0-9]+]]:q2only = SYS_LOG2F_PSEUDO
; MIR-NEXT:  {{%[0-9]+}}:gpr32 = nomerge COPY{{.*}} [[LOG2F]]
; MIR:       [[LOG10F:%[0-9]+]]:q2only = SYS_LOG10F_PSEUDO
; MIR-NEXT:  {{%[0-9]+}}:gpr32 = nomerge COPY{{.*}} [[LOG10F]]
; MIR:       [[POWF:%[0-9]+]]:q2only = SYS_POWF_PSEUDO {{%[0-9]+}}, {{%[0-9]+}}
; MIR-NEXT:  {{%[0-9]+}}:gpr32 = nomerge COPY{{.*}} [[POWF]]
; MIR:       [[HYPOTF:%[0-9]+]]:q2only = SYS_HYPOTF_PSEUDO {{%[0-9]+}}, {{%[0-9]+}}
; MIR-NEXT:  {{%[0-9]+}}:gpr32 = nomerge COPY{{.*}} [[HYPOTF]]
; MIR:       [[FMODF:%[0-9]+]]:q2only = SYS_FMODF_PSEUDO {{%[0-9]+}}, {{%[0-9]+}}
; MIR-NEXT:  {{%[0-9]+}}:gpr32 = nomerge COPY{{.*}} [[FMODF]]

declare float @llvm.sin.f32(float)
declare float @llvm.cos.f32(float)
declare float @llvm.pow.f32(float, float)

define float @standard_math_intrinsics(float %x, float %y) {
; CHECK-LABEL: standard_math_intrinsics:
; CHECK:       sys sinf
; CHECK:       sys cosf
; CHECK:       sys powf
  %sin = call float @llvm.sin.f32(float %x)
  %cos = call float @llvm.cos.f32(float %sin)
  %pow = call float @llvm.pow.f32(float %cos, float %y)
  ret float %pow
}

define float @overlapping_math_results(float %x, float %y) {
; CHECK-LABEL: overlapping_math_results:
; CHECK-DAG:   sys sinf
; CHECK-DAG:   sys powf
; CHECK:       fadd
  %sin = call float @llvm.avm.sinf(float %x)
  %pow = call float @llvm.avm.powf(float %x, float %y)
  %sum = fadd float %sin, %pow
  ret float %sum
}

define float @two_binary_results(float %x, float %y) {
; CHECK-LABEL: two_binary_results:
; CHECK-DAG:   sys powf
; CHECK-DAG:   sys hypotf
; CHECK:       fadd
  %pow = call float @llvm.avm.powf(float %x, float %y)
  %hypot = call float @llvm.avm.hypotf(float %x, float %y)
  %sum = fadd float %pow, %hypot
  ret float %sum
}

define float @chained_services(float %x, float %y) {
; CHECK-LABEL: chained_services:
; CHECK:       sys powf
; CHECK:       sys sinf
  %pow = call float @llvm.avm.powf(float %x, float %y)
  %sin = call float @llvm.avm.sinf(float %pow)
  ret float %sin
}

define i32 @overlapping_timer32_results() {
; CHECK-LABEL: overlapping_timer32_results:
; CHECK:       sys millis32
; CHECK:       sys millis32
; CHECK:       add
  %a = call i32 @llvm.avm.millis32()
  %b = call i32 @llvm.avm.millis32()
  %sum = add i32 %a, %b
  ret i32 %sum
}

define i16 @overlapping_timer16_results() {
; CHECK-LABEL: overlapping_timer16_results:
; CHECK:       sys millis
; CHECK:       sys millis
; CHECK:       add
  %a = call i16 @llvm.avm.millis()
  %b = call i16 @llvm.avm.millis()
  %sum = add i16 %a, %b
  ret i16 %sum
}
