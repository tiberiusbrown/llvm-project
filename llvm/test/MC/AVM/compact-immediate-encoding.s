# RUN: llvm-mc -triple=avm -show-encoding %s | FileCheck %s

ldi8 c0, 0
ldi8 c1, 1
ldi8 c2, 127
ldi8 c3, 128
ldi8 c0, 255
ldi16 c0, 0
ldi16 c1, 1
ldi16 c2, 255
ldi16 c3, 256
ldi16 c0, 0x1234
ldi16 c1, 65535
addi.s8 c0, -128
addi.s8 c1, -127
addi.s8 c2, -1
addi.s8 c3, 0
addi.s8 c0, 1
addi.s8 c1, 126
addi.s8 c2, 127
cmpi.s8 c0, -128
cmpi.s8 c1, -127
cmpi.s8 c2, -1
cmpi.s8 c3, 0
cmpi.s8 c0, 1
cmpi.s8 c1, 126
cmpi.s8 c2, 127

# CHECK: ldi8{{.*}}encoding: [0xc0,0x00]
# CHECK: ldi8{{.*}}encoding: [0xc1,0x01]
# CHECK: ldi8{{.*}}encoding: [0xc2,0x7f]
# CHECK: ldi8{{.*}}encoding: [0xc3,0x80]
# CHECK: ldi8{{.*}}encoding: [0xc0,0xff]
# CHECK: ldi16{{.*}}encoding: [0xc4,0x00,0x00]
# CHECK: ldi16{{.*}}encoding: [0xc5,0x01,0x00]
# CHECK: ldi16{{.*}}encoding: [0xc6,0xff,0x00]
# CHECK: ldi16{{.*}}encoding: [0xc7,0x00,0x01]
# CHECK: ldi16{{.*}}encoding: [0xc4,0x34,0x12]
# CHECK: ldi16{{.*}}encoding: [0xc5,0xff,0xff]
# CHECK: addi.s8{{.*}}encoding: [0xc8,0x80]
# CHECK: addi.s8{{.*}}encoding: [0xc9,0x81]
# CHECK: addi.s8{{.*}}encoding: [0xca,0xff]
# CHECK: addi.s8{{.*}}encoding: [0xcb,0x00]
# CHECK: addi.s8{{.*}}encoding: [0xc8,0x01]
# CHECK: addi.s8{{.*}}encoding: [0xc9,0x7e]
# CHECK: addi.s8{{.*}}encoding: [0xca,0x7f]
# CHECK: cmpi.s8{{.*}}encoding: [0xcc,0x80]
# CHECK: cmpi.s8{{.*}}encoding: [0xcd,0x81]
# CHECK: cmpi.s8{{.*}}encoding: [0xce,0xff]
# CHECK: cmpi.s8{{.*}}encoding: [0xcf,0x00]
# CHECK: cmpi.s8{{.*}}encoding: [0xcc,0x01]
# CHECK: cmpi.s8{{.*}}encoding: [0xcd,0x7e]
# CHECK: cmpi.s8{{.*}}encoding: [0xce,0x7f]

