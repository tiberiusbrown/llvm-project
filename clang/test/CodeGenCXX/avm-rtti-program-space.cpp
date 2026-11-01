// RUN: %clang_cc1 -triple avm -std=c++23 -fcxx-exceptions -fexceptions -fno-rtti -emit-llvm -o - %s | FileCheck %s

// AVM keeps RTTI descriptors, their type names, and the vtables they name in
// program space. The descriptor's two pointer fields are 24-bit program addresses,
// and the vtable address point lies two 24-bit entries past the vtable symbol.

struct Plain {
  int tag;
};

struct Dtor {
  int tag;
  ~Dtor();
};

void throw_plain() { throw Plain{1}; }
void throw_dtor() { throw Dtor{2}; }
void throw_int() { throw 3; }

// The class descriptor and its name are linkonce_odr program-space objects. The
// address point is a plain getelementptr with no inbounds flag.
// CHECK-DAG: @_ZTI5Plain = linkonce_odr {{.*}}addrspace(1) constant { ptr addrspace(1), ptr addrspace(1) } { ptr addrspace(1) getelementptr (ptr addrspace(1), ptr addrspace(1) @_ZTVN10__cxxabiv117__class_type_infoE, i24 2), ptr addrspace(1) @_ZTS5Plain }
// CHECK-DAG: @_ZTS5Plain = linkonce_odr {{.*}}addrspace(1) constant [7 x i8] c"5Plain\00"
// CHECK-DAG: @_ZTI4Dtor = linkonce_odr {{.*}}addrspace(1) constant { ptr addrspace(1), ptr addrspace(1) } { ptr addrspace(1) getelementptr (ptr addrspace(1), ptr addrspace(1) @_ZTVN10__cxxabiv117__class_type_infoE, i24 2), ptr addrspace(1) @_ZTS4Dtor }
// CHECK-DAG: @_ZTS4Dtor = linkonce_odr {{.*}}addrspace(1) constant [6 x i8] c"4Dtor\00"

// The fundamental descriptor for int is emitted locally, not declared as an
// external standard-library object.
// CHECK-DAG: @_ZTIi = linkonce_odr {{.*}}addrspace(1) constant { ptr addrspace(1), ptr addrspace(1) } { ptr addrspace(1) getelementptr (ptr addrspace(1), ptr addrspace(1) @_ZTVN10__cxxabiv123__fundamental_type_infoE, i24 2), ptr addrspace(1) @_ZTSi }

// The runtime's vtables are declared in program space with 24-bit entries. The
// zero-length declaration is not indexed by an inbounds address calculation.
// CHECK-DAG: @_ZTVN10__cxxabiv117__class_type_infoE = external {{.*}}addrspace(1) constant [0 x ptr addrspace(1)]
// CHECK-DAG: @_ZTVN10__cxxabiv123__fundamental_type_infoE = external {{.*}}addrspace(1) constant [0 x ptr addrspace(1)]

// The throw ABI passes the type descriptor and the destructor as program
// addresses. The class without a destructor passes a null program address. The
// declaration follows the first throw, which is why it is checked there.
// CHECK-LABEL: define {{.*}}void @_Z11throw_plainv(
// CHECK: call addrspace(1) void @__cxa_throw(ptr {{.*}}, ptr addrspace(1) @_ZTI5Plain, ptr addrspace(1) null)
// CHECK-DAG: declare {{.*}}void @__cxa_throw(ptr, ptr addrspace(1), ptr addrspace(1))
// CHECK-LABEL: define {{.*}}void @_Z10throw_dtorv(
// CHECK: call addrspace(1) void @__cxa_throw(ptr {{.*}}, ptr addrspace(1) @_ZTI4Dtor, ptr addrspace(1) @_ZN4DtorD1Ev)
// CHECK-LABEL: define {{.*}}void @_Z9throw_intv(
// CHECK: call addrspace(1) void @__cxa_throw(ptr {{.*}}, ptr addrspace(1) @_ZTIi, ptr addrspace(1) null)
