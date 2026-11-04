// RUN: %clang_cc1 -triple avm -std=c23 -emit-llvm -o - %s | FileCheck %s

// Bit-precise integers up to 64 bits use the integer type of their width, with
// one-byte alignment and ceil(N/8) bytes of storage, as the AVM data model
// requires. Wider storage is never produced.

_Static_assert(sizeof(unsigned _BitInt(1)) == 1, "one bit");
_Static_assert(sizeof(_BitInt(9)) == 2, "nine bits");
_Static_assert(sizeof(_BitInt(40)) == 5, "forty bits");
_Static_assert(sizeof(unsigned _BitInt(64)) == 8, "sixty-four bits");
_Static_assert(_Alignof(_BitInt(40)) == 1, "byte alignment");

// CHECK-LABEL: define {{.*}}i40 @pass40(i40 noundef %value)
unsigned _BitInt(40) pass40(unsigned _BitInt(40) value) {
  return value;
}

// CHECK-LABEL: define {{.*}}i9 @sum9(i9 noundef signext %a, i9 noundef signext %b)
_BitInt(9) sum9(_BitInt(9) a, _BitInt(9) b) {
  return a + b;
}

// CHECK-LABEL: define {{.*}}i64 @shift64(i64 noundef %a, i64 noundef %b)
unsigned _BitInt(64) shift64(unsigned _BitInt(64) a, unsigned _BitInt(64) b) {
  return a << b;
}
