# RUN: llvm-mc -triple=avm --show-encoding %s | FileCheck %s
.set shift_amount, 7
lsl16i c0, shift_amount
# CHECK: lsl16i{{[ \t]+}}c0, 7{{.*}}encoding: [0xfa,0x37]
