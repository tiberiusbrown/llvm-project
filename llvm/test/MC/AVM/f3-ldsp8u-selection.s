# RUN: llvm-mc -triple=avm-unknown-arduboyfx -show-encoding %s | FileCheck %s

ldsp8u c0, [sp+0]
ldsp8u c1, [sp+5]
LDSP8U C3, [SP+15]
ldsp8u r0, [sp+0]
ldsp8u r1, [sp+5]
ldsp8u r2, [sp+15]
ldsp8u r3, [sp+16]

# CHECK: ldsp8u r4, [sp+0] ; encoding: [0xf3,0x40]
# CHECK: ldsp8u r5, [sp+5] ; encoding: [0xf3,0x55]
# CHECK: ldsp8u r7, [sp+15] ; encoding: [0xf3,0x7f]
# CHECK: ldsp8u r0, [sp+0] ; encoding: [0xf0,0x18,0x00]
# CHECK: ldsp8u r1, [sp+5] ; encoding: [0xf0,0x19,0x05]
# CHECK: ldsp8u r2, [sp+15] ; encoding: [0xf0,0x1a,0x0f]
# CHECK: ldsp8u r3, [sp+16] ; encoding: [0xf0,0x1b,0x10]
