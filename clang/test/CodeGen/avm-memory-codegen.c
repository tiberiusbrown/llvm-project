// RUN: %clang_cc1 -triple avm-unknown-arduboyfx -target-cpu avm1 \
// RUN:   -tune-cpu avm-interpreter-32u4-v1 -O2 -S -o - %s \
// RUN:   | FileCheck %s

unsigned short word;
const char text[] = "avm";

struct record {
  unsigned char tag;
  unsigned short value;
};
struct record record;

// CHECK-LABEL: read_word:
// CHECK: ldm16 r4, [word]
unsigned short read_word(void) { return word; }

// CHECK-LABEL: write_record:
// CHECK: stm16 [record+1], r4
void write_record(unsigned short value) { record.value = value; }

// CHECK-LABEL: text_address:
// CHECK: ldi16 r4, text
const char *text_address(void) { return text; }

// CHECK-LABEL: read_signed:
// CHECK: ld8u r4, [r4]
// CHECK-NEXT: sext8 r4
signed char read_signed(const signed char *ptr) { return *ptr; }

// CHECK-LABEL: read_unsigned:
// CHECK: ld8u r4, [r4]
unsigned char read_unsigned(const unsigned char *ptr) { return *ptr; }

// CHECK-LABEL: read_volatile:
// CHECK: ld8u r4, [r4]
unsigned char read_volatile(volatile unsigned char *ptr) { return *ptr; }

// CHECK-LABEL: copy_four:
// CHECK-NOT: call
// CHECK: ret
void copy_four(void *dst, const void *src) { __builtin_memcpy(dst, src, 4); }
