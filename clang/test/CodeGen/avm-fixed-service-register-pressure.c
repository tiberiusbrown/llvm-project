// RUN: %clang_cc1 -triple avm-unknown-arduboyfx -target-cpu avm1 \
// RUN:   -tune-cpu avm-interpreter-32u4-v1 -ffreestanding -Os -S \
// RUN:   -mllvm -verify-machineinstrs -o - %s | FileCheck %s

typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned long uint32_t;

static void putc(uint8_t value) { __avm_debug_putc(value); }

static uint8_t hex_digit(uint8_t value) {
  return value < 10 ? (uint8_t)('0' + value)
                    : (uint8_t)('a' + value - 10);
}

static void hex8(uint8_t value) {
  putc(hex_digit((uint8_t)(value >> 4)));
  putc(hex_digit((uint8_t)(value & 15)));
}

static void hex16(uint16_t value) {
  hex8((uint8_t)(value >> 8));
  hex8((uint8_t)value);
}

static void hex32(uint32_t value) {
  hex16((uint16_t)(value >> 16));
  hex16((uint16_t)value);
}

static void line16(uint8_t label, uint16_t value) {
  putc(label);
  putc(':');
  hex16(value);
  putc('\n');
}

static void line32(uint8_t label, uint32_t value) {
  putc(label);
  putc(':');
  hex32(value);
  putc('\n');
}

// CHECK-LABEL: retained_mixed_results:
// CHECK-COUNT-4: sys debug_putc
uint32_t retained_mixed_results(volatile uint16_t *p16,
                                volatile uint32_t *p32) {
  uint16_t a = (uint16_t)(p16[0] + 0x1234);
  uint16_t b = (uint16_t)(p16[1] ^ 0x55aa);
  uint16_t c = (uint16_t)(p16[2] * 3);
  uint32_t d = p32[0] + p32[1];

  line16('a', a);
  line16('b', b);
  line16('c', c);
  line32('d', d);

  return ((uint32_t)a << 16) ^ ((uint32_t)b + c) ^ d;
}

// CHECK-LABEL: retained_wide_results:
// CHECK-COUNT-5: sys debug_putc
uint32_t retained_wide_results(volatile uint32_t *p) {
  uint32_t a = p[0] + 1;
  uint32_t b = p[1] ^ 0x12345678UL;
  uint32_t c = p[2] - 3;
  uint32_t d = p[3] * 5;
  uint32_t e = p[4] + p[5];

  line32('a', a);
  line32('b', b);
  line32('c', c);
  line32('d', d);
  line32('e', e);

  return a ^ (b + c) ^ (d - e);
}
