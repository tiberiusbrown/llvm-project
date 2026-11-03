// RUN: %clang_cc1 -triple avm -std=c++23 -fno-rtti -emit-llvm -o - %s | FileCheck %s

// Without RTTI, a vtable's RTTI entry is null, and a polymorphic class registers no
// descriptor with the linker. Typeid is not available, so no helper or marker is emitted.

struct Poly {
  virtual ~Poly();
  virtual int value() const;
};

Poly::~Poly() = default;
int Poly::value() const { return 1; }

// CHECK-NOT: __avm_rtti_alias
// CHECK-NOT: __avm_typeid_from_object
// CHECK-NOT: .avm.rtti
// CHECK: @_ZTV4Poly = {{.*}}[ptr addrspace(1) null, ptr addrspace(1) null,
