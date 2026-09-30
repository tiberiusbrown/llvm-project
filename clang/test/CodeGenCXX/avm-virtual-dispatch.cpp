// RUN: %clang_cc1 -triple avm -std=c++20 -fno-rtti -emit-llvm -o - %s | FileCheck %s

struct Base {
  virtual int value();
};
struct Derived : Base {
  int value() override;
};

int Base::value() { return 1; }
int Derived::value() { return 2; }
int invoke(Base *object) { return object->value(); }

struct Polymorphic {
  virtual ~Polymorphic();
};
Polymorphic::~Polymorphic() = default;
void destroy(Polymorphic *object) { ::delete object; }

// Vtables and their 24-bit entries reside in program space. Objects keep a
// three-byte vptr, and virtual calls load a function pointer with LDP24.
// CHECK: @_ZTV4Base = {{.*}}addrspace(1) constant { [3 x ptr addrspace(1)] } { [3 x ptr addrspace(1)] [ptr addrspace(1) null, ptr addrspace(1) null, ptr addrspace(1) @_ZN4Base5valueEv] }
// CHECK: @_ZTV7Derived = {{.*}}addrspace(1) constant { [3 x ptr addrspace(1)] } { [3 x ptr addrspace(1)] [ptr addrspace(1) null, ptr addrspace(1) null, ptr addrspace(1) @_ZN7Derived5valueEv] }
// CHECK-LABEL: define {{.*}} @_Z6invokeP4Base(
// CHECK: %vtable = load ptr addrspace(1), ptr
// CHECK: %vfn = getelementptr inbounds ptr addrspace(1), ptr addrspace(1) %vtable, i64 0
// CHECK: load ptr addrspace(1), ptr addrspace(1) %vfn
// CHECK: call {{.*}} addrspace(1) i16 %{{.*}}(
// CHECK-NOT: addrspacecast
// CHECK-LABEL: define {{.*}} @_Z7destroyP11Polymorphic(
// CHECK: %complete-offset.ptr = getelementptr inbounds ptr addrspace(1), ptr addrspace(1) %vtable, i64 -2
