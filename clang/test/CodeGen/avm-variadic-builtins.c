// RUN: %clang_cc1 -triple avm-unknown-arduboyfx -target-cpu avm1 \
// RUN:   -tune-cpu avm-interpreter-32u4-v1 -ffreestanding -O1 -emit-llvm \
// RUN:   -o - %s | FileCheck %s --check-prefix=IR
// RUN: %clang_cc1 -triple avm-unknown-arduboyfx -target-cpu avm1 \
// RUN:   -tune-cpu avm-interpreter-32u4-v1 -ffreestanding -O2 -S \
// RUN:   -mllvm -verify-machineinstrs -o - %s | FileCheck %s --check-prefix=ASM

typedef signed char int8_t;
typedef unsigned short uint16_t;
typedef unsigned long uint32_t;
typedef const char __attribute__((address_space(1))) *progmem_char_ptr;

int snprintf_ram(char *dst, uint16_t size, const char *format, int i, long l,
                 const char *s, progmem_char_ptr ps) {
  return __avm_snprintf(dst, size, format, i, l, s, ps);
}

int snprintf_program(char *dst, uint16_t size, progmem_char_ptr format, int i,
                     long l, const char *s, progmem_char_ptr ps) {
  return __avm_snprintf_P(dst, size, format, i, l, s, ps);
}

int builtin_alias_ram(char *dst, uint16_t size, const char *format, int value) {
  return __builtin_avm_snprintf(dst, size, format, value);
}

int builtin_alias_program(char *dst, uint16_t size, progmem_char_ptr format,
                          int value) {
  return __builtin_avm_snprintf_p(dst, size, format, value);
}

uint32_t drawf_ram(int x, int y, const char *format, unsigned value) {
  return __avm_draw_textf(x, y, format, value);
}

uint32_t drawf_program(int x, int y, progmem_char_ptr format, unsigned value) {
  return __avm_draw_textf_P(x, y, format, value);
}

uint32_t drawf_builtin_alias(int x, int y, const char *format,
                             unsigned value) {
  return __builtin_avm_draw_textf(x, y, format, value);
}

int promoted_arguments(char *dst, uint16_t size, const char *format,
                       int8_t small, float fp) {
  return __avm_snprintf(dst, size, format, small, fp);
}

int program_pointer_followed_by_int(char *dst, uint16_t size,
                                    const char *format,
                                    progmem_char_ptr string, int value) {
  return __avm_snprintf(dst, size, format, string, value);
}

int no_variadic_arguments(char *dst, uint16_t size, const char *format) {
  return __avm_snprintf(dst, size, format);
}

// AVM packs only the unnamed arguments. Their one-byte-aligned layout is:
// int: 2, long: 4, AS0 pointer: 2, AS1 pointer: 3.
// IR-LABEL: define{{.*}} i16 @snprintf_ram
// IR: [[PACK:%.*]] = alloca [11 x i8], align 1
// IR: store i16
// IR: store i32
// IR: store ptr
// IR: store ptr addrspace(1)
// IR: [[RESULT:%.*]] = call i16 @llvm.avm.vsnprintf(
// IR-SAME: ptr {{.*}}, i16 {{.*}}, ptr {{.*}}, ptr {{.*}})
// IR: ret i16 [[RESULT]]

// IR-LABEL: define{{.*}} i16 @snprintf_program
// IR: alloca [11 x i8], align 1
// IR: call i16 @llvm.avm.vsnprintf.p(
// IR-SAME: ptr {{.*}}, i16 {{.*}}, ptr addrspace(1) {{.*}}, ptr {{.*}})

// IR-LABEL: define{{.*}} i16 @builtin_alias_ram
// IR: call i16 @llvm.avm.vsnprintf(

// IR-LABEL: define{{.*}} i16 @builtin_alias_program
// IR: call i16 @llvm.avm.vsnprintf.p(

// IR-LABEL: define{{.*}} i32 @drawf_ram
// IR: alloca [2 x i8], align 1
// IR: call i32 @llvm.avm.draw.textfv(
// IR-SAME: i16 {{.*}}, i16 {{.*}}, ptr {{.*}}, ptr {{.*}},
// IR-SAME: ptr @__avm_text_state, ptr @__avm_framebuffer)

// IR-LABEL: define{{.*}} i32 @drawf_program
// IR: alloca [2 x i8], align 1
// IR: call i32 @llvm.avm.draw.textfv.p(
// IR-SAME: i16 {{.*}}, i16 {{.*}}, ptr addrspace(1) {{.*}}, ptr {{.*}},
// IR-SAME: ptr @__avm_text_state, ptr @__avm_framebuffer)

// IR-LABEL: define{{.*}} i32 @drawf_builtin_alias
// IR: call i32 @llvm.avm.draw.textfv(

// Default promotions produce a two-byte int and AVM's four-byte double.
// IR-LABEL: define{{.*}} i16 @promoted_arguments
// IR: alloca [6 x i8], align 1
// IR: store i16
// IR: store float
// IR: call i16 @llvm.avm.vsnprintf(

// A program pointer occupies exactly three bytes; the following int starts
// immediately afterward, so the complete pack is five bytes.
// IR-LABEL: define{{.*}} i16 @program_pointer_followed_by_int
// IR: alloca [5 x i8], align 1
// IR: store ptr addrspace(1)
// IR: store i16
// IR: call i16 @llvm.avm.vsnprintf(

// Zero unnamed arguments need no temporary stack object.
// IR-LABEL: define{{.*}} i16 @no_variadic_arguments
// IR-NOT: avm.varargs
// IR: call i16 @llvm.avm.vsnprintf(
// IR-SAME: ptr {{.*}}, i16 {{.*}}, ptr {{.*}}, ptr null)

// ASM-LABEL: snprintf_ram:
// ASM-NOT: call
// ASM: sys vsnprintf
// ASM-LABEL: snprintf_program:
// ASM-NOT: call
// ASM: sys vsnprintf_p
// ASM-LABEL: builtin_alias_ram:
// ASM-NOT: call
// ASM: sys vsnprintf
// ASM-LABEL: builtin_alias_program:
// ASM-NOT: call
// ASM: sys vsnprintf_p
// ASM-LABEL: drawf_ram:
// ASM-NOT: call
// ASM: sys draw_textfv
// ASM-LABEL: drawf_program:
// ASM-NOT: call
// ASM: sys draw_textfv_p
// ASM-LABEL: drawf_builtin_alias:
// ASM-NOT: call
// ASM: sys draw_textfv
// ASM-LABEL: promoted_arguments:
// ASM-NOT: call
// ASM: sys vsnprintf
// ASM-LABEL: program_pointer_followed_by_int:
// ASM-NOT: call
// ASM: sys vsnprintf
// ASM-LABEL: no_variadic_arguments:
// ASM-NOT: call
// ASM: sys vsnprintf
