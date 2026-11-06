// RUN: %clang_cc1 -triple avm -std=c++23 -fcxx-exceptions -fexceptions -fno-rtti -emit-llvm-bc -o %t.O0.bc %s
// RUN: llvm-dis -o - %t.O0.bc | FileCheck %s
// RUN: opt -passes=verify -disable-output %t.O0.bc
// RUN: %clang_cc1 -triple avm -std=c++23 -O2 -fcxx-exceptions -fexceptions -fno-rtti -emit-llvm-bc -o %t.O2.bc %s
// RUN: llvm-dis -o - %t.O2.bc | FileCheck %s
// RUN: opt -passes=verify -disable-output %t.O2.bc

// The class with a nontrivial destructor was the case that the bitcode reader
// rejected. Serializing the module must keep the destructor's program address
// with the declared type of __cxa_throw, so the reader sees a matching call. These
// checks read the bitcode back, not the text, at -O0 and -O2.

struct Dtor {
  int tag;
  ~Dtor() {}
};

void throw_dtor() { throw Dtor{2}; }

// CHECK-DAG: @_ZTI4Dtor = {{.*}}addrspace(1) constant { ptr addrspace(1), ptr addrspace(1) } { ptr addrspace(1) getelementptr (i8, ptr addrspace(1) @_ZTVN10__cxxabiv117__class_type_infoE, i24 6), ptr addrspace(1) @_ZTS4Dtor }
// CHECK-DAG: declare {{.*}}void @__cxa_throw(ptr, ptr addrspace(1), ptr addrspace(1))
// CHECK-DAG: @__cxa_throw(ptr {{.*}}, ptr addrspace(1) @_ZTI4Dtor, ptr addrspace(1) @_ZN4DtorD1Ev)
