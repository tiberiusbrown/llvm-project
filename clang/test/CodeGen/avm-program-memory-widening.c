// RUN: %clang_cc1 -triple avm-unknown-arduboyfx -target-cpu avm1 \
// RUN:   -tune-cpu avm-interpreter-32u4-v1 -ffreestanding -O2 -S -o - %s \
// RUN:   | FileCheck %s

typedef unsigned char uint8_t;
typedef unsigned short uint16_t;

#define AVM_PROGMEM __attribute__((address_space(1)))
#define NOINLINE __attribute__((noinline))

// CHECK-LABEL: sum_program_bytes:
// CHECK-DAG:   ldp32 {{.*}}[q{{[0-9]+}}+]
// CHECK-DAG:   ldp16
// CHECK-DAG:   ldp8u
NOINLINE uint16_t sum_program_bytes(const uint8_t AVM_PROGMEM *p, uint16_t n) {
  uint16_t sum = 0;
  do {
    sum += *p++;
  } while (--n != 0);
  return sum;
}

// CHECK-LABEL: sum_program_words:
// CHECK:       ldp32 {{.*}}[q{{[0-9]+}}+]
// CHECK:       ldp16
NOINLINE uint16_t sum_program_words(const uint16_t AVM_PROGMEM *p,
                                    uint16_t n) {
  uint16_t sum = 0;
  do {
    sum += *p++;
  } while (--n != 0);
  return sum;
}

// CHECK-LABEL: load_program_pair:
// CHECK:       ldp16 {{.*}}[q{{[0-9]+}}+]
NOINLINE uint16_t load_program_pair(const uint8_t AVM_PROGMEM *p, uint16_t n) {
  uint16_t sum = 0;
  for (uint16_t i = 0; i != n; ++i) {
    sum += p[0];
    sum += p[1];
    p += 2;
  }
  return sum;
}

// CHECK-LABEL: load_volatile_program_pair:
// CHECK-NOT:   ldp16
// CHECK-NOT:   ldp32
// CHECK:       ldp8u
// CHECK:       ldp8u
NOINLINE uint16_t
load_volatile_program_pair(const volatile uint8_t AVM_PROGMEM *p) {
  uint16_t first = p[0];
  uint16_t second = p[1];
  return first + second;
}
