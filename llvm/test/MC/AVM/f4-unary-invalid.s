# RUN: not llvm-mc -triple=avm %s 2>&1 | FileCheck %s

lsl16.1 c0
lsl16.1 q0
lsl16.1 sp
lsl16.1 pc
lsl16.1 cc
tst8 c0
tst8 q0
tst8 sp
inc16 c0
dec16 q0
not16 1
not16 symbol
not16 [r0]
tst8 1
tst8 [r0]
inc16 symbol
dec16 [r0]
lsl16
lsr16
asr16
lsl16.1
lsl16.1 r0, r1
not16
not16 r0, r1
tst8
tst8 r0, r1
inc16
inc16 r0, r1
dec16
dec16 r0, r1

# CHECK-COUNT-30: error:
