// RUN: %clang_cc1 -triple avm -emit-llvm -O0 -o - %s | FileCheck %s

typedef char const __attribute__((address_space(1))) *progstr;

// CHECK-LABEL: define{{.*}} i16 @direct_builtin(
// CHECK: call i16 @llvm.avm.debug.printfv.p(ptr addrspace(1) {{[^,]+}}, ptr {{[^)]+}})
int direct_builtin(progstr format, __builtin_va_list ap) {
  return __builtin_avm_debug_printfv_p(format, ap);
}

// CHECK-LABEL: define{{.*}} i16 @direct_alias(
// CHECK: call i16 @llvm.avm.debug.printfv.p(ptr addrspace(1) {{[^,]+}}, ptr {{[^)]+}})
int direct_alias(progstr format, __builtin_va_list ap) {
  return __avm_debug_printfv_P(format, ap);
}

// AVM packs variadic values without alignment padding: int=2, long=4,
// AS0 pointer=2, and AS1 pointer=3, for a total of 11 bytes.
// CHECK-LABEL: define{{.*}} i16 @variadic_builtin(
// CHECK: %avm.varargs = alloca [11 x i8], align 1
// CHECK: store i16 %{{.*}}, ptr %{{.*}}, align 1
// CHECK: store i32 %{{.*}}, ptr %{{.*}}, align 1
// CHECK: store ptr %{{.*}}, ptr %{{.*}}, align 1
// CHECK: store ptr addrspace(1) %{{.*}}, ptr %{{.*}}, align 1
// CHECK: call i16 @llvm.avm.debug.printfv.p(ptr addrspace(1) {{[^,]+}}, ptr {{[^)]+}})
int variadic_builtin(progstr format, int value, long wide,
                     char const *ram_string, progstr program_string) {
  return __builtin_avm_debug_printf_p(format, value, wide, ram_string,
                                      program_string);
}

// CHECK-LABEL: define{{.*}} i16 @variadic_alias(
// CHECK: %avm.varargs = alloca [11 x i8], align 1
// CHECK: call i16 @llvm.avm.debug.printfv.p(ptr addrspace(1) {{[^,]+}}, ptr {{[^)]+}})
int variadic_alias(progstr format, int value, long wide,
                   char const *ram_string, progstr program_string) {
  return __avm_debug_printf_P(format, value, wide, ram_string, program_string);
}

// A format with no unnamed arguments passes a null va_list cursor rather than
// reserving a dummy byte.
// CHECK-LABEL: define{{.*}} i16 @no_arguments(
// CHECK: call i16 @llvm.avm.debug.printfv.p(ptr addrspace(1) {{[^,]+}}, ptr null)
int no_arguments(progstr format) {
  return __avm_debug_printf_P(format);
}
