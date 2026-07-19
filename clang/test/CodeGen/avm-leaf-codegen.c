// RUN: %clang --target=avm-unknown-arduboyfx -O2 -fomit-frame-pointer -S %s -o - \
// RUN:   | FileCheck %s

typedef unsigned short uint16_t;

// CHECK-LABEL: add:
// CHECK-NEXT:  {{.*}}%bb.0:
// CHECK-NEXT:  add r4, r5
// CHECK-NEXT:  ret
uint16_t add(uint16_t a, uint16_t b) { return a + b; }
