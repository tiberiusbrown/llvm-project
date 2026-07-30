// RUN: %clang_cc1 -triple avm-unknown-arduboyfx -target-cpu avm1 \
// RUN:   -tune-cpu avm-interpreter-32u4-v1 -ffreestanding -O0 -emit-llvm \
// RUN:   -o - %s | FileCheck %s --check-prefix=IR
// RUN: %clang_cc1 -triple avm-unknown-arduboyfx -target-cpu avm1 \
// RUN:   -tune-cpu avm-interpreter-32u4-v1 -ffreestanding -O2 -S \
// RUN:   -mllvm -verify-machineinstrs -o - %s | FileCheck %s --check-prefix=ASM

typedef unsigned char uint8_t;
typedef unsigned long uint32_t;
typedef const void __attribute__((address_space(1))) *progmem_void_ptr;
typedef const char __attribute__((address_space(1))) *progmem_char_ptr;

void set_font(progmem_void_ptr font) { __avm_set_text_font(font); }
void set_mode(uint8_t mode) { __avm_set_text_mode(mode); }

uint32_t text_ram(int x, int y, const char *string) {
  return __avm_draw_text(x, y, string);
}

uint32_t text_program(int x, int y, progmem_char_ptr string) {
  return __avm_draw_text_P(x, y, string);
}

uint32_t textfv_ram(int x, int y, const char *format, __builtin_va_list args) {
  return __avm_draw_textfv(x, y, format, args);
}

uint32_t textfv_program(int x, int y, progmem_char_ptr format,
                        __builtin_va_list args) {
  return __avm_draw_textfv_P(x, y, format, args);
}

// IR: @__avm_text_state = external global [7 x i8], align 1
// IR: @__avm_framebuffer = external global [1024 x i8], align 1
// IR-LABEL: define{{.*}} void @set_font
// IR: call void @llvm.avm.set.text.font(ptr addrspace(1) {{.*}}, ptr @__avm_text_state)
// IR-LABEL: define{{.*}} void @set_mode
// IR: call void @llvm.avm.set.text.mode(i8 {{.*}}, ptr @__avm_text_state)
// IR-LABEL: define{{.*}} i32 @text_ram
// IR: call i32 @llvm.avm.draw.text(i16 {{.*}}, i16 {{.*}}, ptr {{.*}}, ptr @__avm_text_state, ptr @__avm_framebuffer)
// IR-LABEL: define{{.*}} i32 @text_program
// IR: call i32 @llvm.avm.draw.text.p(i16 {{.*}}, i16 {{.*}}, ptr addrspace(1) {{.*}}, ptr @__avm_text_state, ptr @__avm_framebuffer)
// IR-LABEL: define{{.*}} i32 @textfv_ram
// IR: call i32 @llvm.avm.draw.textfv(i16 {{.*}}, i16 {{.*}}, ptr {{.*}}, ptr {{.*}}, ptr @__avm_text_state, ptr @__avm_framebuffer)
// IR-LABEL: define{{.*}} i32 @textfv_program
// IR: call i32 @llvm.avm.draw.textfv.p(i16 {{.*}}, i16 {{.*}}, ptr addrspace(1) {{.*}}, ptr {{.*}}, ptr @__avm_text_state, ptr @__avm_framebuffer)

// ASM-LABEL: set_font:
// ASM: sys set_text_font
// ASM-LABEL: set_mode:
// ASM: sys set_text_mode
// ASM-LABEL: text_ram:
// ASM: sys draw_text
// ASM-LABEL: text_program:
// ASM: sys draw_text_p
// ASM-LABEL: textfv_ram:
// ASM: sys draw_textfv
// ASM-LABEL: textfv_program:
// ASM: sys draw_textfv_p
