// RUN: %clang_cc1 -triple avm -std=c++23 -fcxx-exceptions -fexceptions -fno-rtti -emit-llvm -o - %s | FileCheck %s

// Exception-only builds keep the default program-space placement for descriptors.
// Without -frtti, a descriptor has no .avm.rtti section and no typeid marker.

struct Thrown {
  int code;
};

void throw_thrown() { throw Thrown{1}; }

// CHECK: @_ZTI6Thrown = linkonce_odr addrspace(1) constant
// CHECK-NOT: section ".avm.rtti
// CHECK-NOT: __avm_rtti_alias
