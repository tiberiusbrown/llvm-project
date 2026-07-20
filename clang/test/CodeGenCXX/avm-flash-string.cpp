// RUN: %clang_cc1 -triple avm -std=c++20 -emit-llvm -o - %s | FileCheck %s
// RUN: %clang_cc1 -triple avm -std=c++20 -fexperimental-new-constant-interpreter -emit-llvm -o - %s | FileCheck %s

#define AS1 __attribute__((address_space(1)))
using flash_ptr = const char AS1 *;

constexpr flash_ptr a = __builtin_avm_flash_string("C++");
constinit flash_ptr b = __builtin_avm_flash_string("C" "++");
struct pair { flash_ptr first, second; };
constexpr pair p = {__builtin_avm_flash_string("C++"),
                    __builtin_avm_flash_string("C++")};

flash_ptr use() { return __builtin_avm_flash_string("C++"); }

// CHECK: @.avm.flashstr.0 = private addrspace(1) constant [4 x i8] c"C++\00", align 1
// CHECK-NOT: @.avm.flashstr.1
// CHECK-NOT: llvm.global_ctors
// CHECK-NOT: addrspacecast
// CHECK-NOT: __cxx_global_var_init
