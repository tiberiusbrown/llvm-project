// RUN: %clang_cc1 -triple avm -std=c++20 -emit-llvm -o - %s | FileCheck %s

struct Base {
  int nonvirtual();
  virtual int virtual_method();
};
struct First {
  virtual int first();
};
struct Derived : First, Base {
  int extra;
};

static_assert(sizeof(int (Base::*)()) == 5);
static_assert(sizeof(int Base::*) == 2);

// CHECK: @nonvirtual = {{.*}}global { i24, i16 } { i24 ptrtoint (ptr addrspace(1) @_ZN4Base10nonvirtualEv to i24), i16 0 }
int (Base::*nonvirtual)() = &Base::nonvirtual;

// CHECK: @virtual_member = {{.*}}global { i24, i16 } { i24 1, i16 0 }
int (Base::*virtual_member)() = &Base::virtual_method;

// CHECK: @null_member = {{.*}}global { i24, i16 } zeroinitializer
int (Base::*null_member)() = nullptr;

int invoke(Base *object, int (Base::*member)()) {
  // CHECK-LABEL: define {{.*}} @_Z6invokeP4BaseMS_FivE(
  // CHECK: %memptr.ptr = extractvalue { i24, i16 } {{.*}}, 0
  // CHECK: and i24 %memptr.ptr, 1
  // CHECK: %memptr.nonvirtualfn = inttoptr i24 %memptr.ptr to ptr addrspace(1)
  return (object->*member)();
}

int (Derived::*convert(int (Base::*member)()))() {
  // CHECK-LABEL: define {{.*}} @_Z7convertM4BaseFivE(
  // CHECK: extractvalue { i24, i16 } {{.*}}, 1
  // CHECK: insertvalue { i24, i16 }
  return member;
}
