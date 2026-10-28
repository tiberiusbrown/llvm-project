// RUN: %clang_cc1 -triple avm -emit-llvm -o - %s | FileCheck %s
// RUN: %clang_cc1 -triple avm -x c++ -std=c++17 -emit-llvm -o - %s | FileCheck %s

#ifdef __cplusplus
extern "C"
#endif
const char __attribute__((address_space(1))) *function_name(void) {
  return __builtin_avm_flash_string(__func__);
}

// CHECK: @.avm.flashstr.0 = private addrspace(1) constant [14 x i8] c"function_name\00", align 1
// CHECK-NOT: @__func__
// CHECK-LABEL: define{{.*}} ptr addrspace(1) @function_name(
// CHECK: ret ptr addrspace(1) @.avm.flashstr.0
