# RUN: llvm-mc -triple=avm -show-encoding %s | FileCheck %s

ldi8 c0, 1+2
ldi8 c1, 'A'
ldi8 c2, 'P'
ldi8 c3, '\n'
ldi8 c0, '\''
ldi8 c1, '\\'
ldi16 c2, 0x1200+0x34
ldi16 c3, 'P'
addi.s8 c3, -(4+1)
cmpi.s8 c0, 64-65

# CHECK: ldi8{{.*}}encoding: [0xc0,0x03]
# CHECK: ldi8{{.*}}encoding: [0xc1,0x41]
# CHECK: ldi8{{.*}}encoding: [0xc2,0x50]
# CHECK: ldi8{{.*}}encoding: [0xc3,0x0a]
# CHECK: ldi8{{.*}}encoding: [0xc0,0x27]
# CHECK: ldi8{{.*}}encoding: [0xc1,0x5c]
# CHECK: ldi16{{.*}}encoding: [0xc6,0x34,0x12]
# CHECK: ldi16{{.*}}encoding: [0xc7,0x50,0x00]
# CHECK: addi.s8{{.*}}encoding: [0xcb,0xfb]
# CHECK: cmpi.s8{{.*}}encoding: [0xcc,0xff]
