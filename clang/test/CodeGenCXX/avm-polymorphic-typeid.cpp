// RUN: %clang_cc1 -triple avm -std=c++23 -emit-llvm -o - %s | FileCheck %s

// A typeid that needs a run-time lookup through a vptr is lowered to the AVM runtime helper,
// which reads the object's RTTI slot and returns the descriptor's flash alias. Static typeid of
// a polymorphic class keeps its constant alias.

namespace std {
class type_info {
public:
  virtual ~type_info();
};
}

struct Base {
  virtual ~Base() = default;
};

struct Derived : Base {};

// CHECK-LABEL: define {{.*}}@_Z7inspectR4Base(
// CHECK:       call addrspace(1) ptr @__avm_typeid_from_object(ptr %{{[0-9]+}})
const std::type_info &inspect(Base &value) {
  return typeid(value);
}

// CHECK-LABEL: define {{.*}}@_Z9keep_basev(
// CHECK-NEXT:  entry:
// CHECK-NEXT:  ret ptr inttoptr (i16 trunc (i32 add (i32 ptrtoint (ptr addrspace(1) @_ZTI4Base to i32), i32 1536) to i16) to ptr)
const std::type_info &keep_base() { return typeid(Base); }

// CHECK-LABEL: define {{.*}}@_Z12keep_derivedv(
// CHECK-NEXT:  entry:
// CHECK-NEXT:  ret ptr inttoptr (i16 trunc (i32 add (i32 ptrtoint (ptr addrspace(1) @_ZTI7Derived to i32), i32 1536) to i16) to ptr)
const std::type_info &keep_derived() { return typeid(Derived); }
