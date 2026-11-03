// RUN: %clang_cc1 -triple avm -std=c++23 -fcxx-exceptions -fexceptions -Wno-potentially-evaluated-expression -emit-llvm -o - %s | FileCheck %s

// A dynamic typeid on AVM reads the object's vptr through the runtime helper, which
// returns the AS0 std::type_info pointer. The vptr is a 24-bit program address, so the
// generic vtable load, which reads a 16-bit RTTI slot, is never emitted. A null pointer
// operand branches to __cxa_bad_typeid, which is potentially throwing.

namespace std {
class type_info {
public:
  virtual ~type_info();
  const char *name() const;
};
class bad_typeid {
public:
  virtual ~bad_typeid();
};
}

struct Base {
  virtual ~Base() = default;
};

struct Derived : Base {};

Base &counted();

struct Plain {
  int p;
};

int plain_calls;
Plain &plain();

Plain &count_plain() {
  ++plain_calls;
  return plain();
}

// A dynamic reference passes its AS0 object pointer to the helper, and returns the result.
// CHECK-LABEL: define {{.*}}@_Z17dynamic_referenceR4Base(
// CHECK:       %[[OBJ:[0-9]+]] = load ptr, ptr %b.addr
// CHECK-NEXT:  %{{[0-9]+}} = call addrspace(1) ptr @__avm_typeid_from_object(ptr %[[OBJ]]) #{{[0-9]+}}
// CHECK-NEXT:  ret ptr
const std::type_info &dynamic_reference(Base &b) { return typeid(b); }

// The helper is declared with an AS0 object pointer and an AS0 result, at its first use.
// CHECK: declare ptr @__avm_typeid_from_object(ptr) addrspace(1)

// A null pointer dereference branches to __cxa_bad_typeid, which is a noreturn call with no
// nounwind attribute, so the exception can propagate. The nonnull path calls the helper.
// CHECK-LABEL: define {{.*}}@_Z15dynamic_pointerP4Base(
// CHECK:       icmp eq ptr %{{[0-9]+}}, null
// CHECK:       typeid.bad_typeid:
// CHECK-NEXT:  call addrspace(1) void @__cxa_bad_typeid() #[[BAD:[0-9]+]]
// CHECK-NEXT:  unreachable
// CHECK:       typeid.end:
// CHECK-NEXT:  %{{[0-9]+}} = call addrspace(1) ptr @__avm_typeid_from_object(ptr %{{[0-9]+}}) #{{[0-9]+}}
const std::type_info &dynamic_pointer(Base *p) { return typeid(*p); }

// The bad-typeid call is declared without nounwind, at its first use.
// CHECK: declare void @__cxa_bad_typeid() addrspace(1)

// Inside a try block, the same null check is an invoke whose unwind edge reaches the
// handler for std::bad_typeid.
// CHECK-LABEL: define {{.*}}@_Z7guardedP4Base(
// CHECK:       invoke addrspace(1) void @__cxa_bad_typeid()
// CHECK:       landingpad
// CHECK:       catch ptr addrspace(1) @_ZTISt10bad_typeid
int guarded(Base *p) {
  try {
    (void)typeid(*p);
  } catch (const std::bad_typeid &) {
    return 1;
  }
  return 0;
}

// A statically known complete object keeps the constant alias, with no helper call.
// CHECK-LABEL: define {{.*}}@_Z14static_derivedv(
// CHECK-NEXT:  entry:
// CHECK-NEXT:  ret ptr inttoptr (i16 trunc (i32 add (i32 ptrtoint (ptr addrspace(1) @_ZTI7Derived to i32), i32 1536) to i16) to ptr)
const std::type_info &static_derived() { return typeid(Derived); }

// The operand of a nonpolymorphic typeid is unevaluated, so count_plain is never called.
// The result is the static alias of its type.
// CHECK-LABEL: define {{.*}}@_Z14nonpolymorphicv(
// CHECK-NEXT:  entry:
// CHECK-NEXT:  ret ptr inttoptr (i16 trunc (i32 add (i32 ptrtoint (ptr addrspace(1) @_ZTI5Plain to i32), i32 1536) to i16) to ptr)
const std::type_info &nonpolymorphic() { return typeid(count_plain()); }

// A polymorphic operand is evaluated exactly once, and its result is the helper's input.
// CHECK-LABEL: define {{.*}}@_Z4oncev(
// CHECK:       %call = call {{.*}}@_Z7countedv()
// CHECK-NEXT:  %{{[0-9]+}} = call addrspace(1) ptr @__avm_typeid_from_object(ptr %call)
const std::type_info &once() { return typeid(counted()); }

// The bad-typeid call is noreturn and keeps no nounwind attribute, so it can unwind.
// CHECK: attributes #[[BAD]] = { noreturn }
