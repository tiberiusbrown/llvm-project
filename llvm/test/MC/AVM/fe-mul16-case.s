# RUN: llvm-mc -triple=avm --show-encoding %s | FileCheck %s

MUL16 R7,R0
MuL16 r4,R5
mul16 r0,r7

# CHECK: mul16 r7, r0{{.*}}encoding: [0xfe,0x38]
# CHECK: mul16 r4, r5{{.*}}encoding: [0xfe,0x25]
# CHECK: mul16 r0, r7{{.*}}encoding: [0xfe,0x07]
