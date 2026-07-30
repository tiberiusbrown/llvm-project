; RUN: llc -mtriple=avm -O0 -verify-machineinstrs < %s | FileCheck %s --check-prefix=ASM
; RUN: llc -mtriple=avm -O2 -verify-machineinstrs < %s | FileCheck %s --check-prefix=ASM
; RUN: llc -mtriple=avm -O0 -stop-after=avm-system-service-regions < %s -o - \
; RUN:   | FileCheck %s --check-prefix=MIR

@__avm_text_state = external global [7 x i8], align 1
@__avm_framebuffer = external global [1024 x i8], align 1
@font = external addrspace(1) global [3 x i8], align 1
@ram_text = internal constant [4 x i8] c"abc\00", align 1
@program_text = internal addrspace(1) constant [4 x i8] c"abc\00", align 1
@ram_format = internal constant [3 x i8] c"%u\00", align 1
@program_format = internal addrspace(1) constant [3 x i8] c"%S\00", align 1

declare void @llvm.avm.set.text.font(ptr addrspace(1), ptr)
declare void @llvm.avm.set.text.mode(i8, ptr)
declare i32 @llvm.avm.draw.text(i16, i16, ptr, ptr, ptr)
declare i32 @llvm.avm.draw.text.p(i16, i16, ptr addrspace(1), ptr, ptr)
declare i32 @llvm.avm.draw.textfv(i16, i16, ptr, ptr, ptr, ptr)
declare i32 @llvm.avm.draw.textfv.p(i16, i16, ptr addrspace(1), ptr, ptr, ptr)

define void @set_text_state(i8 %mode) {
; ASM-LABEL: set_text_state:
; ASM:       sys set_text_font
; ASM:       sys set_text_mode
; MIR-LABEL: name: set_text_state
; MIR:       SYS_SET_TEXT_FONT_PSEUDO
; MIR-SAME:  :: (load (s24) from @font, align 1, addrspace 1), (store (s56) into @__avm_text_state, align 1)
; MIR:       SYS_SET_TEXT_MODE_PSEUDO
; MIR-SAME:  :: (store (s56) into @__avm_text_state, align 1)
  call void @llvm.avm.set.text.font(
      ptr addrspace(1) @font, ptr @__avm_text_state)
  call void @llvm.avm.set.text.mode(i8 %mode, ptr @__avm_text_state)
  ret void
}

define i32 @draw_ram(i16 %x, i16 %y) {
; ASM-LABEL: draw_ram:
; ASM:       sys draw_text
; MIR-LABEL: name: draw_ram
; MIR:       [[CURSOR:%[0-9]+]]:q2only = SYS_DRAW_TEXT_PSEUDO
; MIR-SAME:  :: (load unknown-size from @ram_text, align 1), (load (s56) from @__avm_text_state, align 1), (load (s8192) from @__avm_framebuffer, align 1), (store (s8192) into @__avm_framebuffer, align 1), (load unknown-size, align 1, addrspace 1)
  %cursor = call i32 @llvm.avm.draw.text(
      i16 %x, i16 %y, ptr @ram_text, ptr @__avm_text_state,
      ptr @__avm_framebuffer)
  ret i32 %cursor
}

define i32 @draw_program(i16 %x, i16 %y) {
; ASM-LABEL: draw_program:
; ASM:       sys draw_text_p
; MIR-LABEL: name: draw_program
; MIR-NOT:   PROG_CANON_PSEUDO
; MIR:       [[CURSOR:%[0-9]+]]:q2only = SYS_DRAW_TEXT_P_PSEUDO
  %cursor = call i32 @llvm.avm.draw.text.p(
      i16 %x, i16 %y, ptr addrspace(1) @program_text,
      ptr @__avm_text_state, ptr @__avm_framebuffer)
  ret i32 %cursor
}

define i32 @drawfv_ram(i16 %x, i16 %y, ptr %ap) {
; ASM-LABEL: drawfv_ram:
; ASM:       sys draw_textfv
; MIR-LABEL: name: drawfv_ram
; MIR:       [[CURSOR:%[0-9]+]]:q2only = SYS_DRAW_TEXTFV_PSEUDO
  %cursor = call i32 @llvm.avm.draw.textfv(
      i16 %x, i16 %y, ptr @ram_format, ptr %ap,
      ptr @__avm_text_state, ptr @__avm_framebuffer)
  ret i32 %cursor
}

define i32 @drawfv_program(i16 %x, i16 %y, ptr %ap) {
; ASM-LABEL: drawfv_program:
; ASM:       sys draw_textfv_p
; MIR-LABEL: name: drawfv_program
; MIR-NOT:   PROG_CANON_PSEUDO
; MIR:       [[CURSOR:%[0-9]+]]:q2only = SYS_DRAW_TEXTFV_P_PSEUDO
  %cursor = call i32 @llvm.avm.draw.textfv.p(
      i16 %x, i16 %y, ptr addrspace(1) @program_format, ptr %ap,
      ptr @__avm_text_state, ptr @__avm_framebuffer)
  ret i32 %cursor
}

define i32 @unnormalized_text_pointers(i16 %x, i16 %y, i32 %bits, ptr %ap) {
; ASM-LABEL: unnormalized_text_pointers:
; ASM:       sys set_text_font
; ASM:       sys draw_text_p
; ASM:       sys draw_textfv_p
; MIR-LABEL: name: unnormalized_text_pointers
; MIR-NOT:   PROG_CANON_PSEUDO
; MIR:       SYS_SET_TEXT_FONT_PSEUDO
; MIR:       SYS_DRAW_TEXT_P_PSEUDO
; MIR:       SYS_DRAW_TEXTFV_P_PSEUDO
  %pointer = inttoptr i32 %bits to ptr addrspace(1)
  call void @llvm.avm.set.text.font(
      ptr addrspace(1) %pointer, ptr @__avm_text_state)
  %a = call i32 @llvm.avm.draw.text.p(
      i16 %x, i16 %y, ptr addrspace(1) %pointer,
      ptr @__avm_text_state, ptr @__avm_framebuffer)
  %b = call i32 @llvm.avm.draw.textfv.p(
      i16 %x, i16 %y, ptr addrspace(1) %pointer, ptr %ap,
      ptr @__avm_text_state, ptr @__avm_framebuffer)
  %result = xor i32 %a, %b
  ret i32 %result
}
