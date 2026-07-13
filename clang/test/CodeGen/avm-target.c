// RUN: %clang_cc1 -triple avm-unknown-arduboyfx -emit-llvm -o - %s | FileCheck %s

// CHECK: target datalayout = "e-m:e-p:16:8-p1:24:8-i8:8-i16:8-i32:8-i64:8-f16:8-f32:8-n8:16-S8"
// CHECK: target triple = "avm-unknown-arduboyfx"
// CHECK: define{{.*}} i16 @add_and_test(i16 noundef %a, i16 noundef %b)
// CHECK: alloca i16, align 1
// CHECK: add{{.*}} i16
// CHECK: icmp eq i16
// CHECK: define{{.*}} ptr addrspace(1) @program_pointer(ptr addrspace(1) noundef %p)

int add_and_test(int a, int b) {
  int result = a + b;
  return result == 7 ? result : 0;
}

typedef int __attribute__((address_space(1))) program_int;

program_int *program_pointer(program_int *p) { return p; }
