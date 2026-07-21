// RUN: %clang --target=avm-unknown-arduboyfx -ffreestanding -O2 \
// RUN:   -fomit-frame-pointer \
// RUN:   -S %s -o - | FileCheck %s

typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef long int32_t;
typedef const uint8_t program_char __attribute__((address_space(1)));
typedef uint16_t (*callback_t)(uint16_t);

program_char message[] = "AVM";

// CHECK-LABEL: add32_values:
// CHECK:       add32
int32_t add32_values(int32_t left, int32_t right) { return left + right; }

// CHECK-LABEL: add_float_values:
// CHECK:       fadd
float add_float_values(float left, float right) { return left + right; }

// CHECK-LABEL: read_program_char:
// CHECK:       add32
// CHECK-NOT:   zext8
// CHECK:       ldp8u
uint8_t read_program_char(program_char *base, int32_t index) {
  return base[index];
}

uint16_t callback(uint16_t value) { return value; }
callback_t callback_slot = callback;

// CHECK-LABEL: call_loaded_callback:
// CHECK:       ld16
// CHECK:       ld8u
// CHECK:       jmpp
uint16_t call_loaded_callback(uint16_t value) {
  return callback_slot(value);
}

// CHECK:       .type callback_slot,@object
// CHECK:       .progptr %prog24(callback)
// CHECK:       .size callback_slot, 3
