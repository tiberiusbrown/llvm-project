# RUN: llvm-mc -triple=avm -show-encoding %s | FileCheck %s

stsp8 [sp+0], c0
stsp8 [sp+15], c3
stsp8 [sp+0], r4
stsp8 [sp+15], r7
stsp8 [sp+16], r4
stsp8 [sp+(2+3)], c0
stsp8 [sp+8-1], c1
stsp8 [sp+(4*3)], c2

# CHECK: encoding: [0xf1,0x30]
# CHECK: encoding: [0xf1,0x6f]
# CHECK: encoding: [0xf0,0x2c,0x00]
# CHECK: encoding: [0xf0,0x2f,0x0f]
# CHECK: encoding: [0xf0,0x2c,0x10]
# CHECK: encoding: [0xf1,0x44]
# CHECK: encoding: [0xf1,0x4d]
# CHECK: encoding: [0xf1,0x62]
