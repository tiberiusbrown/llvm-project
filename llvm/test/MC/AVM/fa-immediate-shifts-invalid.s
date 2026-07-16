# RUN: not llvm-mc -triple=avm %s 2>&1 | FileCheck %s
# CHECK-COUNT-6: error:
lsl16i c0, -1
lsl16i c0, 16
lsr16i c2, 0x100
asr16i c3, symbol
lsl16i r0, 4
lsl16i q0, 4
