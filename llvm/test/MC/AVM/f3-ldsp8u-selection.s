# RUN: llvm-mc -triple=avm-unknown-arduboyfx -show-encoding %s | FileCheck %s

ldsp8u c0, [sp+0]
ldsp8u c1, [sp+5]
LDSP8U C3, [SP+15]
ldsp8u r4, [sp+0]
ldsp8u r5, [sp+5]
ldsp8u r6, [sp+15]
ldsp8u r7, [sp+16]

# CHECK: ldsp8u c0, [sp+0] ; encoding: [0xf3,0x40]
# CHECK: ldsp8u c1, [sp+5] ; encoding: [0xf3,0x55]
# CHECK: ldsp8u c3, [sp+15] ; encoding: [0xf3,0x7f]
# CHECK: ldsp8u r4, [sp+0] ; encoding: [0xf0,0x1c,0x00]
# CHECK: ldsp8u r5, [sp+5] ; encoding: [0xf0,0x1d,0x05]
# CHECK: ldsp8u r6, [sp+15] ; encoding: [0xf0,0x1e,0x0f]
# CHECK: ldsp8u r7, [sp+16] ; encoding: [0xf0,0x1f,0x10]

