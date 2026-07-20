// RUN: %clang_cc1 -triple avm -fsyntax-only -verify %s

#define AS1 __attribute__((address_space(1)))

void valid_constraints(unsigned short word, unsigned char byte,
                       unsigned long pair, const void AS1 *program,
                       unsigned short *memory) {
  __asm__ volatile("" : : "r"(word), "c"(word), "b"(byte), "B"(byte),
                   "p"(memory), "P"(memory), "q"(pair), "Q"(pair),
                   "t"(program));
  __asm__ volatile("" : : "I"(-128), "J"(255), "K"(15), "L"(-32768),
                   "M"(65535U), "N"(15), "O"(0));
  __asm__ volatile("" : : "m"(*memory), "o"(*memory) : "cc", "memory");
  __asm__ volatile("" : : "{r0}"(word), "{r1}"(word), "{r2}"(word),
                   "{r3}"(word), "{r4}"(word), "{r5}"(word),
                   "{r6}"(word), "{r7}"(word), "{q0}"(pair),
                   "{q1}"(pair), "{q2}"(pair), "{q3}"(pair));
}

void reserved_registers(unsigned short value) {
  __asm__("" : "={sp}"(value)); // expected-error {{invalid output constraint '={sp}' in asm}}
  __asm__("" : "={pc}"(value)); // expected-error {{invalid output constraint '={pc}' in asm}}
  __asm__("" : : : "sp"); // expected-error {{unknown register name 'sp' in asm}}
  __asm__("" : : : "pc"); // expected-error {{unknown register name 'pc' in asm}}
}
