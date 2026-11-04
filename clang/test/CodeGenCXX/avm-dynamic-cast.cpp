// RUN: %clang_cc1 -triple avm -std=c++23 -fcxx-exceptions -fexceptions -O0 -emit-llvm -o - %s | FileCheck %s --check-prefixes=CHECK,O0
// RUN: %clang_cc1 -triple avm -std=c++23 -fcxx-exceptions -fexceptions -O2 -emit-llvm -o - %s | FileCheck %s --check-prefixes=CHECK,O2
// RUN: %clang_cc1 -triple x86_64-unknown-linux-gnu -std=c++23 -fcxx-exceptions -fexceptions -O2 -emit-llvm -o - %s | FileCheck %s --check-prefix=X86

// On AVM a dynamic_cast to a class calls the runtime __dynamic_cast, whose object pointer is
// 16-bit and whose RTTI arguments are 24-bit program addresses. The hint is a signed 16-bit value.
// A dynamic_cast to void* calls __avm_dynamic_cast_to_void, which needs only the offset-to-top.
// A failed reference cast calls __cxa_bad_cast, which is potentially throwing. A final class uses
// the runtime and not the exact vptr comparison, because AVM vptrs are 24-bit.

namespace std {
class type_info {
public:
  virtual ~type_info();
};
class bad_cast {
public:
  bad_cast();
  virtual ~bad_cast();
};
}

struct Base {
  virtual ~Base() = default;
};

struct Derived : Base {};

struct Final final : Base {};

// CHECK-LABEL: define {{.*}}to_derived
Derived *to_derived(Base *p) {
  // CHECK: call addrspace(1) ptr @__dynamic_cast(ptr {{.*}}, ptr addrspace(1) @_ZTI4Base, ptr addrspace(1) @_ZTI7Derived, i16 0)
  return dynamic_cast<Derived *>(p);
}

// A reference cast that fails throws, so the failure path calls __cxa_bad_cast.
// CHECK-LABEL: define {{.*}}ref_of
Derived &ref_of(Base &b) {
  // CHECK: call addrspace(1) ptr @__dynamic_cast(ptr {{.*}}, ptr addrspace(1) @_ZTI4Base, ptr addrspace(1) @_ZTI7Derived, i16 0)
  // CHECK: call addrspace(1) void @__cxa_bad_cast()
  return dynamic_cast<Derived &>(b);
}

// A cast to void* guards the null source, and the call takes the 16-bit object pointer.
// CHECK-LABEL: define {{.*}}void_of
void *void_of(Base *p) {
  // O0: icmp eq ptr %{{.*}}, null
  // CHECK: call addrspace(1) ptr @__avm_dynamic_cast_to_void(ptr {{.*}})
  return dynamic_cast<void *>(p);
}

// A final class is a runtime cast on AVM, so no exact vptr comparison is emitted for it.
// CHECK-LABEL: define {{.*}}cast_to_final
Final *cast_to_final(Base *p) {
  // O2: call addrspace(1) ptr @__dynamic_cast(ptr {{.*}}, ptr addrspace(1) @_ZTI4Base, ptr addrspace(1) @_ZTI5Final, i16 0)
  // O0: call addrspace(1) ptr @__dynamic_cast(ptr {{.*}}, ptr addrspace(1) @_ZTI4Base, ptr addrspace(1) @_ZTI5Final, i16 0)
  return dynamic_cast<Final *>(p);
}

// A derived-to-base dynamic_cast is an implicit conversion, so it calls no runtime function.
// CHECK-LABEL: define {{.*}}upcast
Base *upcast(Derived *d) {
  // CHECK-NOT: __dynamic_cast
  // CHECK-NOT: __avm_dynamic_cast_to_void
  // CHECK: ret ptr
  return dynamic_cast<Base *>(d);
}

// Non-AVM targets keep the generic lowering. The final class uses its exact vptr comparison at
// O2, and dynamic_cast<void*> reads the offset-to-top from the vtable, not the runtime helper.
// X86-LABEL: define {{.*}}void_of
// X86-NOT: __avm_dynamic_cast_to_void
// X86: ret ptr
// X86-LABEL: define {{.*}}cast_to_final
// X86: _ZTV5Final
