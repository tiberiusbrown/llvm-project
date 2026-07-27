; RUN: llc -mtriple=avm -O0 -verify-machineinstrs < %s | FileCheck %s --check-prefixes=CHECK,CHECK-O0
; RUN: llc -mtriple=avm -O2 -verify-machineinstrs < %s | FileCheck %s
; RUN: llc -mtriple=avm -O0 -stop-after=avm-system-service-regions < %s -o - \
; RUN:   | FileCheck %s --check-prefixes=MIR,MIR-O0
; RUN: llc -mtriple=avm -O2 -stop-after=avm-system-service-regions < %s -o - \
; RUN:   | FileCheck %s --check-prefixes=MIR,MIR-O2

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
declare void @llvm.avm.display(i16)
@__avm_framebuffer = external global [1024 x i8], align 1
@sprite_object = external addrspace(1) global [8 x i8], align 1

declare void @llvm.avm.draw.sprite.overwrite(i16, i16, ptr addrspace(1), i16,
                                              ptr)
declare void @llvm.avm.draw.sprite.plus.mask(i16, i16, ptr addrspace(1), i16,
                                              ptr)
declare void @llvm.avm.draw.sprite.self.masked(i16, i16, ptr addrspace(1), i16,
                                                ptr)
declare void @llvm.avm.draw.sprite.erase(i16, i16, ptr addrspace(1), i16, ptr)

declare void @llvm.avm.draw.filled.rect.white(i16, i16, i8, i8, ptr)
declare void @llvm.avm.draw.filled.rect.black(i16, i16, i8, i8, ptr)

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

define void @display_service(i16 %clear) {
; CHECK-LABEL: display_service:
; CHECK:       sys display
  call void @llvm.avm.display(i16 %clear)
  ret void
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

define void @display_and_sprite_services(i16 %x, i16 %y,
                                         ptr addrspace(1) %sprite,
                                         i16 %frame) {
; CHECK-LABEL: display_and_sprite_services:
; CHECK-O0:    sys display
; CHECK:       sys draw_sprite_overwrite
; CHECK:       sys draw_sprite_plus_mask
; CHECK:       sys draw_sprite_self_masked
; CHECK:       sys draw_sprite_erase
  call void @llvm.avm.display(i16 0)
  call void @llvm.avm.draw.sprite.overwrite(i16 %x, i16 %y,
                                             ptr addrspace(1) %sprite,
                                             i16 %frame,
                                             ptr @__avm_framebuffer)
  call void @llvm.avm.draw.sprite.plus.mask(i16 %x, i16 %y,
                                             ptr addrspace(1) %sprite,
                                             i16 %frame,
                                             ptr @__avm_framebuffer)
  call void @llvm.avm.draw.sprite.self.masked(i16 %x, i16 %y,
                                               ptr addrspace(1) %sprite,
                                               i16 %frame,
                                               ptr @__avm_framebuffer)
  call void @llvm.avm.draw.sprite.erase(i16 %x, i16 %y,
                                         ptr addrspace(1) %sprite,
                                         i16 %frame,
                                         ptr @__avm_framebuffer)
  ret void
}

define void @filled_rect_services(i16 %x, i16 %y, i8 %width, i8 %height) {
; CHECK-LABEL: filled_rect_services:
; CHECK:       sys draw_filled_rect_white
; CHECK:       sys draw_filled_rect_black
  call void @llvm.avm.draw.filled.rect.white(
      i16 %x, i16 %y, i8 %width, i8 %height,
      ptr @__avm_framebuffer)
  call void @llvm.avm.draw.filled.rect.black(
      i16 %x, i16 %y, i8 %width, i8 %height,
      ptr @__avm_framebuffer)
  ret void
}

define void @timer_result_as_sprite_frame(i16 %x, i16 %y,
                                          ptr addrspace(1) %sprite) {
  %frame = call i16 @llvm.avm.millis()
  call void @llvm.avm.draw.sprite.overwrite(i16 %x, i16 %y,
                                             ptr addrspace(1) %sprite,
                                             i16 %frame,
                                             ptr @__avm_framebuffer)
  ret void
}

define void @inttoptr_sprite_unnormalized(i32 %bits) {
  %sprite = inttoptr i32 %bits to ptr addrspace(1)
  call void @llvm.avm.draw.sprite.overwrite(
      i16 1, i16 2, ptr addrspace(1) %sprite, i16 3,
      ptr @__avm_framebuffer)
  ret void
}

define void @global_sprite_object_mmo() {
  call void @llvm.avm.draw.sprite.overwrite(
      i16 1, i16 2, ptr addrspace(1) @sprite_object, i16 3,
      ptr @__avm_framebuffer)
  ret void
}

; MIR-LABEL: name: debug_services
; MIR:       SYS_DEBUG_PUTC_PSEUDO
; MIR:       SYS_DEBUG_BREAK_PSEUDO
; MIR-LABEL: name: timer_result_to_debug
; MIR:       [[DEBUG_RESULT:%[0-9]+]]:gpr16 = SYS_MILLIS_PSEUDO
; MIR:       [[DEBUG_CHARACTER:%[0-9]+]]:gpr16 = ZEXT8_PSEUDO [[DEBUG_RESULT]]
; MIR:       SYS_DEBUG_PUTC_PSEUDO{{.*}} [[DEBUG_CHARACTER]]
; MIR-LABEL: name: timer_services
; MIR:       {{%[0-9]+}}:gpr16 = SYS_MILLIS_PSEUDO
; MIR-NEXT:  [[TIMER:%[0-9]+]]:gpr16 = SYS_MILLIS_PSEUDO
; MIR-LABEL: name: timer32_service
; MIR:       [[TIMER32:%[0-9]+]]:gpr32 = SYS_MILLIS32_PSEUDO
; MIR-LABEL: name: math_services
; MIR-O2:    [[SINF:%[0-9]+]]:q2only = SYS_SINF_PSEUDO
; MIR-O2:    [[COSF:%[0-9]+]]:q2only = SYS_COSF_PSEUDO
; MIR-O2:    [[ATAN2F:%[0-9]+]]:q2only = SYS_ATAN2F_PSEUDO {{%[0-9]+}}, {{%[0-9]+}}
; MIR-O2:    [[TANF:%[0-9]+]]:q2only = SYS_TANF_PSEUDO
; MIR-O2:    [[EXPF:%[0-9]+]]:q2only = SYS_EXPF_PSEUDO
; MIR-O2:    [[LOGF:%[0-9]+]]:q2only = SYS_LOGF_PSEUDO
; MIR-O2:    [[LOG2F:%[0-9]+]]:q2only = SYS_LOG2F_PSEUDO
; MIR-O2:    [[LOG10F:%[0-9]+]]:q2only = SYS_LOG10F_PSEUDO
; MIR-O2:    [[POWF:%[0-9]+]]:q2only = SYS_POWF_PSEUDO {{%[0-9]+}}, {{%[0-9]+}}
; MIR-O2:    [[HYPOTF:%[0-9]+]]:q2only = SYS_HYPOTF_PSEUDO {{%[0-9]+}}, {{%[0-9]+}}
; MIR-O2:    [[FMODF:%[0-9]+]]:gpr32 = SYS_FMODF_PSEUDO {{%[0-9]+}}, {{%[0-9]+}}
; MIR-LABEL: name: display_service
; MIR:       SYS_DISPLAY_PSEUDO {{%[0-9]+}} :: (load (s8192) from @__avm_framebuffer, align 1), (store (s8192) into @__avm_framebuffer, align 1)
; MIR-LABEL: name: display_and_sprite_services
; MIR-O0:    SYS_DISPLAY_PSEUDO {{%[0-9]+}} :: (load (s8192) from @__avm_framebuffer, align 1), (store (s8192) into @__avm_framebuffer, align 1)
; MIR-O2:    [[X:%[0-9]+]]:r4only = nomerge COPY
; MIR-O2:    [[Y:%[0-9]+]]:r5only = nomerge COPY
; MIR-O2:    [[SPRITE:%[0-9]+]]:q3only = nomerge COPY
; MIR-O2:    [[FRAME:%[0-9]+]]:r0only = nomerge COPY
; MIR-O0:    SYS_DRAW_SPRITE_OVERWRITE_PSEUDO{{.*}} :: (load (s8192) from @__avm_framebuffer, align 1), (store (s8192) into @__avm_framebuffer, align 1), (load unknown-size, align 1, addrspace 1)
; MIR-O2:    SYS_DRAW_SPRITE_OVERWRITE_PSEUDO [[X]], [[Y]], [[SPRITE]], [[FRAME]] :: (load (s8192) from @__avm_framebuffer, align 1), (store (s8192) into @__avm_framebuffer, align 1), (load unknown-size, align 1, addrspace 1)
; MIR:       SYS_DRAW_SPRITE_PLUS_MASK_PSEUDO{{.*}} :: (load (s8192) from @__avm_framebuffer, align 1), (store (s8192) into @__avm_framebuffer, align 1), (load unknown-size, align 1, addrspace 1)
; MIR:       SYS_DRAW_SPRITE_SELF_MASKED_PSEUDO{{.*}} :: (load (s8192) from @__avm_framebuffer, align 1), (store (s8192) into @__avm_framebuffer, align 1), (load unknown-size, align 1, addrspace 1)
; MIR:       SYS_DRAW_SPRITE_ERASE_PSEUDO{{.*}} :: (load (s8192) from @__avm_framebuffer, align 1), (store (s8192) into @__avm_framebuffer, align 1), (load unknown-size, align 1, addrspace 1)
; MIR-LABEL: name: filled_rect_services
; MIR:       SYS_DRAW_FILLED_RECT_WHITE_PSEUDO{{.*}} :: (load (s8192) from @__avm_framebuffer, align 1), (store (s8192) into @__avm_framebuffer, align 1)
; MIR:       SYS_DRAW_FILLED_RECT_BLACK_PSEUDO{{.*}} :: (load (s8192) from @__avm_framebuffer, align 1), (store (s8192) into @__avm_framebuffer, align 1)
; MIR-LABEL: name: timer_result_as_sprite_frame
; MIR:       [[SPRITE_TIMER:%[0-9]+]]:gpr16 = SYS_MILLIS_PSEUDO
; MIR:       SYS_DRAW_SPRITE_OVERWRITE_PSEUDO {{%[0-9]+}}, {{%[0-9]+}}, {{(killed )?}}{{%[0-9]+}}, {{(killed )?}}[[SPRITE_TIMER]]
; MIR-LABEL: name: inttoptr_sprite_unnormalized
; MIR-NOT:   PROG_CANON_PSEUDO
; MIR:       SYS_DRAW_SPRITE_OVERWRITE_PSEUDO
; MIR-LABEL: name: global_sprite_object_mmo
; MIR:       SYS_DRAW_SPRITE_OVERWRITE_PSEUDO{{.*}} :: (load (s8192) from @__avm_framebuffer, align 1), (store (s8192) into @__avm_framebuffer, align 1), (load unknown-size from @sprite_object, align 1, addrspace 1)

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
