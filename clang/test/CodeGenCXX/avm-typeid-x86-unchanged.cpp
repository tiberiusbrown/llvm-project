// RUN: %clang_cc1 -triple x86_64-unknown-linux-gnu -std=c++23 -emit-llvm -o - %s | FileCheck %s

// The non-AVM Itanium lowering is unchanged. A dynamic typeid loads the RTTI pointer from
// the vtable's slot one entry before the address point, and it never calls the AVM helper.

namespace std {
class type_info {
public:
  virtual ~type_info();
};
}

struct Base {
  virtual ~Base() = default;
};

// CHECK-LABEL: define {{.*}}@_Z17dynamic_referenceR4Base(
// CHECK:       %[[VTABLE:.*]] = load ptr, ptr %{{[0-9]+}}
// CHECK-NEXT:  %{{[0-9]+}} = getelementptr inbounds ptr, ptr %[[VTABLE]], i64 -1
// CHECK-NEXT:  %{{[0-9]+}} = load ptr, ptr %{{[0-9]+}}
// CHECK-NEXT:  ret ptr
// CHECK-NOT:   __avm_typeid_from_object
const std::type_info &dynamic_reference(Base &b) { return typeid(b); }
