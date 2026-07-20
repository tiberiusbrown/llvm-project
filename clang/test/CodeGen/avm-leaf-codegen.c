// RUN: %clang --target=avm-unknown-arduboyfx -ffreestanding -O2 \
// RUN:   -fomit-frame-pointer -S %s -o - \
// RUN:   | FileCheck %s

typedef unsigned short uint16_t;

// CHECK-LABEL: add:
// CHECK-NEXT:  {{.*}}%bb.0:
// CHECK-NEXT:  add r4, r5
// CHECK-NEXT:  ret
uint16_t add(uint16_t a, uint16_t b) { return a + b; }

// CHECK-LABEL: inc:
// CHECK-NEXT:  {{.*}}%bb.0:
// CHECK-NEXT:  inc16 r4
// CHECK-NEXT:  ret
uint16_t inc(uint16_t value) { return value + 1; }
