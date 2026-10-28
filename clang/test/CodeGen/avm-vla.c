// RUN: %clang_cc1 -triple avm -std=c99 -emit-llvm -o - %s | FileCheck %s

void consume(void *);
// CHECK-LABEL: define{{.*}} @scopes(
// CHECK: call addrspace(1) ptr @llvm.stacksave
// CHECK: alloca i16, i16
// CHECK: call addrspace(1) void @consume
// CHECK: call addrspace(1) void @llvm.stackrestore
void scopes(unsigned n) {
  { unsigned short a[n]; a[n - 1] = 42; consume(a); }
  { unsigned long b[n]; consume(b); }
}

// CHECK-LABEL: define{{.*}} @size(
// CHECK: mul nuw i16 2,
unsigned size(unsigned n) {
  unsigned short a[n];
  return sizeof a;
}

