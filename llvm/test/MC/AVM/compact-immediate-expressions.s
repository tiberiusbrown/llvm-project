# RUN: llvm-mc -triple=avm -show-encoding %s | FileCheck %s

ldi8 c0, 1+2
ldi8 c1, 'A'
ldi16 c2, 0x1200+0x34
addi.s8 c3, -(4+1)
cmpi.s8 c0, 64-65

# CHECK: ldi8{{.*}}encoding: [0xc0,0x03]
# CHECK: ldi8{{.*}}encoding: [0xc1,0x41]
# CHECK: ldi16{{.*}}encoding: [0xc6,0x34,0x12]
# CHECK: addi.s8{{.*}}encoding: [0xcb,0xfb]
# CHECK: cmpi.s8{{.*}}encoding: [0xcc,0xff]

