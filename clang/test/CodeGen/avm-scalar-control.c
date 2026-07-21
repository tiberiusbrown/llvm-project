// RUN: %clang --target=avm-unknown-arduboyfx -ffreestanding -O0 \
// RUN:   -fomit-frame-pointer -S %s -o - \
// RUN:   | FileCheck %s --check-prefix=O0
// RUN: %clang --target=avm-unknown-arduboyfx -ffreestanding -O2 \
// RUN:   -fomit-frame-pointer -S %s -o - \
// RUN:   | FileCheck %s --check-prefix=O2
// RUN: %clang --target=avm-unknown-arduboyfx -ffreestanding -Os \
// RUN:   -fomit-frame-pointer -S %s -o - \
// RUN:   | FileCheck %s --check-prefix=OS
// RUN: %clang --target=avm-unknown-arduboyfx -ffreestanding -Oz \
// RUN:   -fomit-frame-pointer -S %s -o - \
// RUN:   | FileCheck %s --check-prefix=OZ

typedef unsigned char u8;
typedef signed char s8;
typedef unsigned short u16;
typedef signed short s16;
typedef unsigned long u32;

// O0-LABEL: avm_if:
// O0:       cmp
// O0:       br
// O2-LABEL: avm_if:
// O2:       cmp
// O2-NEXT:  cmov.ult
u16 avm_if(u16 a, u16 b) {
  if (a < b)
    return a + 1;
  return a - 1;
}

// O2-LABEL: avm_loop:
// O2:       tst16
// O2:       br
u16 avm_loop(volatile u16 *out, u16 n) {
  u16 i;
  for (i = 0; i < n; ++i)
    *out = i;
  return i;
}

// O2-LABEL: avm_zero8:
// O2:       tst8
u16 avm_zero8(u8 value) { return value == 0; }

// O2-LABEL: avm_zero16:
// O2:       tst16
u16 avm_zero16(u16 value) { return value != 0; }

// O2-LABEL: avm_signed:
// O2:       cset.slt
u16 avm_signed(s16 a, s16 b) { return a < b; }

// O2-LABEL: avm_unsigned:
// O2:       cset.uge
u16 avm_unsigned(u16 a, u16 b) { return a >= b; }

// O2-LABEL: avm_bool:
// O2:       cmpi.s8
// O2-NEXT:  cset.eq
u16 avm_bool(u16 a) { return a == 42; }

// O0-LABEL: avm_select:
// O0:       cmp
// O0:       br
// O2-LABEL: avm_select:
// O2:       cmov.slt
// OS-LABEL: avm_select:
// OS:       cmov.slt
// OZ-LABEL: avm_select:
// OZ:       cmov.slt
s16 avm_select(s16 a, s16 b, s16 x, s16 y) { return a < b ? x : y; }

// O2-LABEL: avm_select32:
// O2-COUNT-2: cmov.ult
// OS-LABEL: avm_select32:
// OS-COUNT-2: cmov.ult
// OZ-LABEL: avm_select32:
// OZ-COUNT-2: cmov.ult
u32 avm_select32(u16 a, u16 b, u32 x, u32 y) { return a < b ? x : y; }

// O2-LABEL: avm_switch:
// O2-NOT:   jmpp
// O2:       cmp
// O2:       br
u16 avm_switch(u16 value, u16 a, u16 b) {
  switch (value) {
  case 0:
    return a;
  case 1:
    return b;
  case 2:
    return a + 3;
  case 3:
    return b - 5;
  case 4:
    return a ^ b;
  default:
    return 99;
  }
}

#define SHIFT_FUNCTIONS(N)                                                   \
  u16 shl##N(u16 x) { return x << N; }                                      \
  u16 lshr##N(u16 x) { return x >> N; }                                     \
  s16 ashr##N(s16 x) { return x >> N; }

SHIFT_FUNCTIONS(0)

// O2-LABEL: shl1:
// O2:       add r4, r4
// OS-LABEL: shl1:
// OS:       lsl16i r4, 1
// OZ-LABEL: shl1:
// OZ:       lsl16i r4, 1
// O2-LABEL: lshr1:
// O2:       lsr16.1 r4
// OS-LABEL: lshr1:
// OS:       lsr16i r4, 1
// OZ-LABEL: lshr1:
// OZ:       lsr16i r4, 1
// O2-LABEL: ashr1:
// O2:       asr16.1 r4
// OS-LABEL: ashr1:
// OS:       asr16i r4, 1
// OZ-LABEL: ashr1:
// OZ:       asr16i r4, 1
SHIFT_FUNCTIONS(1)

// O2-LABEL: lshr2:
// O2:       lsr16i r4, 2
SHIFT_FUNCTIONS(2)

// O2-LABEL: shl3:
// O2-COUNT-3: add r4, r4
SHIFT_FUNCTIONS(3)

// O2-LABEL: shl4:
// O2:       lsl16i r4, 4
SHIFT_FUNCTIONS(4)
SHIFT_FUNCTIONS(5)
SHIFT_FUNCTIONS(6)
SHIFT_FUNCTIONS(7)
SHIFT_FUNCTIONS(8)
SHIFT_FUNCTIONS(9)
SHIFT_FUNCTIONS(10)
SHIFT_FUNCTIONS(11)
SHIFT_FUNCTIONS(12)
SHIFT_FUNCTIONS(13)
SHIFT_FUNCTIONS(14)
SHIFT_FUNCTIONS(15)

// O2-LABEL: shl_variable:
// O2:       shl16v
u16 shl_variable(u16 x, u16 count) { return x << count; }

// O2-LABEL: lshr_variable:
// O2:       lsr16v
u16 lshr_variable(u16 x, u16 count) { return x >> count; }

// O2-LABEL: ashr_variable:
// O2:       asr16v
s16 ashr_variable(s16 x, u16 count) { return x >> count; }

// O2-LABEL: multiply8:
// O2:       mulu8.w
u8 multiply8(u8 a, u8 b) { return a * b; }

// O2-LABEL: widening_multiply:
// O2:       mulu8.w
u16 widening_multiply(u8 a, u8 b) { return (u16)a * (u16)b; }

// O2-LABEL: multiply16:
// O2:       mul16
u16 multiply16(u16 a, u16 b) { return a * b; }

// O2-LABEL: divide16:
// O2:       udiv16
u16 divide16(u16 a, u16 b) { return a / b; }

// O2-LABEL: remainder16:
// O2:       urem16
u16 remainder16(u16 a, u16 b) { return a % b; }

// O2-LABEL: signed_divide16:
// O2:       sdiv16
s16 signed_divide16(s16 a, s16 b) { return a / b; }

// O2-LABEL: signed_remainder16:
// O2:       srem16
s16 signed_remainder16(s16 a, s16 b) { return a % b; }
