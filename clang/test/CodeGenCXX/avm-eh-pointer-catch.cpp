// RUN: %clang_cc1 -triple avm -std=c++23 -fcxx-exceptions -fexceptions -fno-rtti -emit-llvm -o - %s | FileCheck %s

// On AVM, __cxa_begin_catch returns the exception object's address, which is a data
// pointer. A catch of a pointer type therefore loads the caught pointer from that
// object, and the load keeps the pointer's own address space. A function pointer is
// 24 bits in program space, so it is read as a 24-bit value. The return of
// __cxa_begin_catch is never cast across address spaces, which AVM does not lower.
// A by-value catch with a copy constructor calls __cxa_get_exception_ptr before it
// begins the catch, so the runtime must provide that function as well.

void handler(int) {}
int target;

// CHECK-LABEL: define {{.*}} @_Z22catch_function_pointerv(
// CHECK:       %[[ADJUSTED:[0-9]+]] = call addrspace(1) ptr @__cxa_begin_catch(
// CHECK-NEXT:  %exn.casted = load ptr addrspace(1), ptr %[[ADJUSTED]]
int catch_function_pointer() {
  try {
    throw &handler;
  } catch (void (*fp)(int)) {
    fp(4);
    return 1;
  }
  return 0;
}

// CHECK-LABEL: define {{.*}} @_Z18catch_data_pointerv(
// CHECK:       %[[ADJUSTED2:[0-9]+]] = call addrspace(1) ptr @__cxa_begin_catch(
// CHECK-NEXT:  %exn.casted{{[0-9]*}} = load ptr, ptr %[[ADJUSTED2]]
int catch_data_pointer() {
  try {
    throw &target;
  } catch (int *p) {
    return *p;
  }
  return 0;
}

struct Tracked {
  int tag;
  Tracked(int t) : tag(t) {}
  Tracked(const Tracked &other) : tag(other.tag) {}
  ~Tracked() {}
};

// CHECK-LABEL: define {{.*}} @_Z18catch_by_copy_ctorv(
// CHECK:       call addrspace(1) ptr @__cxa_get_exception_ptr(
int catch_by_copy_ctor() {
  try {
    throw Tracked(3);
  } catch (Tracked copy) {
    return copy.tag;
  }
  return 0;
}
