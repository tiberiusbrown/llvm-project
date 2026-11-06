// RUN: %clang_cc1 -triple avm -std=c++23 -fcxx-exceptions -fexceptions -fno-rtti -emit-llvm -o - %s | FileCheck %s

// Pointer descriptors carry the representation of the pointer they describe. The
// flag word's AVM bit, 0x80, marks a 24-bit pointer: a function pointer, or a pointer
// into program memory. A data pointer leaves it clear. A catch of a pointer that is
// not const-qualified, by reference, names a marker that matches the exact pointer type.

#define AVM_PROGMEM __attribute__((address_space(1)))

struct Base {
  int b;
};

struct Derived : Base {
  int d;
};

void throw_derived(Derived *p) { throw p; }
void throw_program(const int AVM_PROGMEM *q) { throw q; }
void throw_noexcept(void (*fp)() noexcept) { throw fp; }
void throw_plain_fn(void (*fp)()) { throw fp; }

// A data pointer descriptor, with a pointee that is a class, and no AVM bit.
// CHECK-DAG: @_ZTIP4Base = linkonce_odr {{.*}}addrspace(1) constant { ptr addrspace(1), ptr addrspace(1), i16, ptr addrspace(1) } { ptr addrspace(1) getelementptr (i8, ptr addrspace(1) @_ZTVN10__cxxabiv119__pointer_type_infoE, i24 6), ptr addrspace(1) @_ZTSP4Base, i16 0, ptr addrspace(1) @_ZTI4Base }

// A pointer into program memory is 24 bits, so the flag word is 0x81 for a const int.
// CHECK-DAG: @_ZTIPU3AS1Ki = linkonce_odr {{.*}}addrspace(1) constant { ptr addrspace(1), ptr addrspace(1), i16, ptr addrspace(1) } { ptr addrspace(1) getelementptr (i8, ptr addrspace(1) @_ZTVN10__cxxabiv119__pointer_type_infoE, i24 6), ptr addrspace(1) @_ZTSPU3AS1Ki, i16 129, ptr addrspace(1) @_ZTIi }

// A function pointer is 24 bits, and a noexcept function pointer adds the noexcept bit 0x40,
// so its flag word is 0xc0. Its pointee is the same function descriptor as the plain type.
// CHECK-DAG: @_ZTIPDoFvvE = linkonce_odr {{.*}}addrspace(1) constant { ptr addrspace(1), ptr addrspace(1), i16, ptr addrspace(1) } { ptr addrspace(1) getelementptr (i8, ptr addrspace(1) @_ZTVN10__cxxabiv119__pointer_type_infoE, i24 6), ptr addrspace(1) @_ZTSPDoFvvE, i16 192, ptr addrspace(1) @_ZTIFvvE }
// CHECK-DAG: @_ZTIPFvvE = linkonce_odr {{.*}}addrspace(1) constant { ptr addrspace(1), ptr addrspace(1), i16, ptr addrspace(1) } { ptr addrspace(1) getelementptr (i8, ptr addrspace(1) @_ZTVN10__cxxabiv119__pointer_type_infoE, i24 6), ptr addrspace(1) @_ZTSPFvvE, i16 128, ptr addrspace(1) @_ZTIFvvE }

// The marker for a non-const pointer object is named after its reference type. Its three
// fields are the marker support vtable's address point, three reserved zero bytes, and the
// underlying pointer descriptor. It is linkonce_odr with a comdat, and it is program-space.
// CHECK-DAG: @_ZTIRP4Base = linkonce_odr {{.*}}addrspace(1) constant { ptr addrspace(1), [3 x i8], ptr addrspace(1) } { ptr addrspace(1) getelementptr (i8, ptr addrspace(1) @__avm_eh_exact_pointer_ref_type_info, i24 6), [3 x i8] zeroinitializer, ptr addrspace(1) @_ZTIP4Base }, comdat, align 1
// CHECK-DAG: @_ZTIRPU3AS1Ki = linkonce_odr {{.*}}addrspace(1) constant { ptr addrspace(1), [3 x i8], ptr addrspace(1) } { ptr addrspace(1) getelementptr (i8, ptr addrspace(1) @__avm_eh_exact_pointer_ref_type_info, i24 6), [3 x i8] zeroinitializer, ptr addrspace(1) @_ZTIPU3AS1Ki }, comdat, align 1
// The runtime's marker vtable is declared in program space, as the other support vtables are.
// CHECK-DAG: @__avm_eh_exact_pointer_ref_type_info = external {{.*}}addrspace(1) constant [0 x ptr addrspace(1)]

// The thrown pointer keeps the ordinary descriptor of its own type.
// CHECK-LABEL: define {{.*}}void @_Z13throw_derivedP7Derived(
// CHECK:       call addrspace(1) void @__cxa_throw(ptr {{.*}}, ptr addrspace(1) @_ZTIP7Derived,

// A non-const pointer caught by reference names the marker. The landing pad matches the
// marker, and __cxa_begin_catch returns the address of the caught pointer object, which
// the reference binds to directly. No temporary holds a copy of the pointer.
// CHECK-LABEL: define {{.*}} @_Z21catch_mutable_pointerv(
// CHECK:       catch ptr addrspace(1) @_ZTIRP4Base
// CHECK:       %[[ADJUSTED:[0-9]+]] = call addrspace(1) ptr @__cxa_begin_catch(
// CHECK-NOT:   exn.byref.tmp
// CHECK-NEXT:  store ptr %[[ADJUSTED]], ptr %r
int catch_mutable_pointer() {
  try {
    throw_derived(nullptr);
  } catch (Base *&r) {
    return r == nullptr ? 1 : 2;
  }
  return 0;
}

// A const pointer object is caught through the ordinary descriptor, which permits the
// pointer conversions, so no marker is emitted for it.
// CHECK-LABEL: define {{.*}} @_Z19catch_const_pointerv(
// CHECK:       catch ptr addrspace(1) @_ZTIP4Base
// CHECK-NOT:   @_ZTIRP4Base
int catch_const_pointer() {
  try {
    throw_derived(nullptr);
  } catch (Base *const &r) {
    return r == nullptr ? 1 : 2;
  }
  return 0;
}

// A reference to a program-memory pointer uses the marker with its 24-bit underlying
// descriptor.
// CHECK-LABEL: define {{.*}} @_Z17catch_program_refv(
// CHECK:       catch ptr addrspace(1) @_ZTIRPU3AS1Ki
int catch_program_ref() {
  try {
    throw_program(nullptr);
  } catch (const int AVM_PROGMEM *&q) {
    return q == nullptr ? 1 : 2;
  }
  return 0;
}
