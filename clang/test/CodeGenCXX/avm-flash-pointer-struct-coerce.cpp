// RUN: %clang_cc1 -triple avm -std=c++20 -O0 -emit-llvm -o - %s | FileCheck %s
// RUN: %clang_cc1 -triple avm -std=c++20 -O2 -emit-llvm -o - %s | FileCheck %s

// A struct whose member is an address-space-1 (flash) pointer is coerced to an integer when it is
// passed by value. Flash pointers are 24 bits, so the coercion must not truncate them to the
// 16-bit width of the default address space.

#define AS1 __attribute__((address_space(1)))

struct Holder {
  const char AS1 *text;
};

struct RamHolder {
  const char *text;
};

static const char AS1 TEXT[] = "abcdefghij";

// CHECK-LABEL: define {{.*}}first_by_value{{.*}}(i24
// CHECK: inttoptr i24 {{.*}} to ptr addrspace(1)
// CHECK-NOT: 65535
__attribute__((noinline)) int first_by_value(Holder h, unsigned char i) {
  return h.text[i];
}

// CHECK-LABEL: define {{.*}}first_ram{{.*}}(i16
__attribute__((noinline)) int first_ram(RamHolder h, unsigned char i) {
  return h.text[i];
}

// The flash pointer is converted at its own width, not truncated to 16 bits. The RAM pointer in the
// same call is legitimately 16 bits wide.
// CHECK-LABEL: define {{.*}}use_both
// CHECK: ptrtoint {{.*}} to i24
// CHECK-NOT: ptrtoint ptr addrspace(1) {{.*}} to i16
// CHECK: ret i16
int use_both(unsigned char i) {
  return first_by_value(Holder{TEXT}, i) + first_ram(RamHolder{"ram"}, i);
}
