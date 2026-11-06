// RUN: %clang_cc1 -triple avm -std=c++23 -O0 -emit-llvm -o - %s | FileCheck %s

// A class's vtable names its RTTI descriptor, which a dynamic typeid can reach from any
// object of that vtable. Emitting the vtable therefore registers the descriptor and its name
// with the linker, as a static typeid would, even when the program never names the type.
// The construction vtables of a class with virtual bases name the class under construction,
// so each of those descriptors is registered too.

// Key is never named by typeid, so only its vtable registers its descriptor.
struct Key {
  virtual ~Key();
  virtual int value() const;
};
Key::~Key() = default;
int Key::value() const { return 3; }

struct VBase {
  virtual ~VBase() = default;
  int vb;
};

struct VLeft : virtual VBase {
  int vl;
};

struct VRight : virtual VBase {
  int vr;
};

struct VTop : VLeft, VRight {
  int vt;
};

void make_top() {
  VTop top;
  (void)top;
}

// Each vtable's RTTI entry is the descriptor's program-space address.
// CHECK-DAG: ".weak __avm_rtti_alias._ZTI3Key"
// CHECK-DAG: ".weak __avm_rtti_alias._ZTS3Key"
// CHECK-DAG: ".weak __avm_rtti_alias._ZTI5VLeft"
// CHECK-DAG: ".weak __avm_rtti_alias._ZTI6VRight"
// CHECK-DAG: ".weak __avm_rtti_alias._ZTI4VTop"
// CHECK-DAG: ".weak __avm_rtti_alias._ZTI5VBase"
// CHECK-DAG: ".weak __avm_rtti_alias._ZTS5VBase"

// CHECK: @_ZTV3Key = {{.*}}[ptr addrspace(1) null, ptr addrspace(1) @_ZTI3Key,

// The construction vtable for VLeft inside VTop names VLeft, and the construction vtable for
// VRight names VRight. Both descriptors are registered by their vtables.
// CHECK: @_ZTC4VTop0_5VLeft = {{.*}}ptr addrspace(1) @_ZTI5VLeft,
// CHECK: @_ZTC4VTop5_6VRight = {{.*}}ptr addrspace(1) @_ZTI6VRight,
