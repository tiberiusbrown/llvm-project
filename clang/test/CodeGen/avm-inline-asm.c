// RUN: %clang_cc1 -triple avm-unknown-arduboyfx -target-cpu avm1 \
// RUN:   -tune-cpu avm-interpreter-32u4-v1 -O0 -S -o - %s | FileCheck %s

#define AS1 __attribute__((address_space(1)))

void register_constraints(unsigned short word, unsigned char byte,
                          unsigned long pair, const void AS1 *program) {
  __asm__ volatile("nop ; %0" : : "r"(word));
  __asm__ volatile("nop ; %0" : : "c"(word));
  __asm__ volatile("nop ; %0" : : "b"(byte));
  __asm__ volatile("nop ; %0" : : "B"(byte));
  __asm__ volatile("nop ; %0" : : "p"(word));
  __asm__ volatile("nop ; %0" : : "P"(word));
  __asm__ volatile("nop ; %0" : : "q"(pair));
  __asm__ volatile("nop ; %0" : : "Q"(pair));
  __asm__ volatile("nop ; %0" : : "t"(program));
}

// CHECK-LABEL: register_constraints:
// CHECK: nop ; r{{[0-7]}}
// CHECK: nop ; r{{[4-7]}}
// CHECK: nop ; r{{[0-7]}}
// CHECK: nop ; r{{[4-7]}}
// CHECK: nop ; r{{[0-7]}}
// CHECK: nop ; r{{[4-7]}}
// CHECK: nop ; q{{[0-3]}}
// CHECK: nop ; q{{[2-3]}}
// CHECK: nop ; q{{[0-3]}}

void fixed_and_immediate(unsigned short word, unsigned long pair) {
  __asm__ volatile("nop ; %0" : : "{r0}"(word));
  __asm__ volatile("nop ; %0" : : "{r7}"(word));
  __asm__ volatile("nop ; %0" : : "{q0}"(pair));
  __asm__ volatile("nop ; %0" : : "{q3}"(pair));
  __asm__ volatile("nop ; %0" : : "I"(-128));
  __asm__ volatile("nop ; %0" : : "J"(255));
  __asm__ volatile("nop ; %0" : : "K"(15));
  __asm__ volatile("nop ; %0" : : "L"(-32768));
  __asm__ volatile("nop ; %0" : : "M"(65535U));
  __asm__ volatile("nop ; %0" : : "N"(15));
  __asm__ volatile("nop ; %0" : : "O"(0));
}

// CHECK-LABEL: fixed_and_immediate:
// CHECK: nop ; r0
// CHECK: nop ; r7
// CHECK: nop ; q0
// CHECK: nop ; q3
// CHECK: nop ; -128
// CHECK: nop ; 255
// CHECK: nop ; 15
// CHECK: nop ; -32768
// CHECK: nop ; 65535
// CHECK: nop ; 15
// CHECK: nop ; 0

void memory_constraints(unsigned short *address) {
  __asm__ volatile("nop ; %0" : : "m"(*address) : "cc", "memory");
  __asm__ volatile("nop ; %0" : : "o"(*address));
}

// CHECK-LABEL: memory_constraints:
// CHECK: nop ; [r{{[0-7]}}]
// CHECK: nop ; [r{{[0-7]}}]
