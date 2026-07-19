// RUN: %clang --target=avm-unknown-arduboyfx -O2 -fomit-frame-pointer \
// RUN:   -S %s -o - | FileCheck %s

typedef unsigned short uint16_t;

extern uint16_t six(uint16_t, uint16_t, uint16_t, uint16_t, uint16_t,
                    uint16_t);

// CHECK-LABEL: caller:
// CHECK:       push16 r1
// CHECK-NEXT:  push16 r0
// CHECK:       adjsp -4
// CHECK-NEXT:  stsp16 [sp+0], r0
// CHECK-NEXT:  stsp16 [sp+2], r1
// CHECK-NEXT:  call six
// CHECK-NEXT:  adjsp 4
// CHECK-NEXT:  pop16 r0
// CHECK-NEXT:  pop16 r1
// CHECK-NEXT:  ret
uint16_t caller(uint16_t a, uint16_t b, uint16_t c, uint16_t d, uint16_t e,
                uint16_t f) {
  return six(a, b, c, d, e, f);
}

typedef uint16_t (*function_pointer)(uint16_t);

// CHECK-LABEL: indirect:
// CHECK:       mov32 q0, q2
// CHECK:       callp q0
// CHECK:       ret
uint16_t indirect(function_pointer fn, uint16_t value) { return fn(value); }
