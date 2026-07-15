# RUN: llvm-mc -triple=avm --show-encoding %s | FileCheck %s

cmp r0, r0
cmp r0, r1
cmp r0, r2
cmp r0, r3
cmp r0, r4
cmp r0, r5
cmp r0, r6
cmp r0, r7
cmp r1, r0
cmp r1, r1
cmp r1, r2
cmp r1, r3
cmp r1, r4
cmp r1, r5
cmp r1, r6
cmp r1, r7
cmp r2, r0
cmp r2, r1
cmp r2, r2
cmp r2, r3
cmp r2, r4
cmp r2, r5
cmp r2, r6
cmp r2, r7
cmp r3, r0
cmp r3, r1
cmp r3, r2
cmp r3, r3
cmp r3, r4
cmp r3, r5
cmp r3, r6
cmp r3, r7
cmp r4, r0
cmp r4, r1
cmp r4, r2
cmp r4, r3
cmp r5, r0
cmp r5, r1
cmp r5, r2
cmp r5, r3
cmp r6, r0
cmp r6, r1
cmp r6, r2
cmp r6, r3
cmp r7, r0
cmp r7, r1
cmp r7, r2
cmp r7, r3

# CHECK: cmp r0, r0{{.*}}encoding: [0xf5,0x00]
# CHECK: cmp r0, r1{{.*}}encoding: [0xf5,0x01]
# CHECK: cmp r0, r2{{.*}}encoding: [0xf5,0x02]
# CHECK: cmp r0, r3{{.*}}encoding: [0xf5,0x03]
# CHECK: cmp r0, r4{{.*}}encoding: [0xf5,0x04]
# CHECK: cmp r0, r5{{.*}}encoding: [0xf5,0x05]
# CHECK: cmp r0, r6{{.*}}encoding: [0xf5,0x06]
# CHECK: cmp r0, r7{{.*}}encoding: [0xf5,0x07]
# CHECK: cmp r1, r0{{.*}}encoding: [0xf5,0x08]
# CHECK: cmp r1, r1{{.*}}encoding: [0xf5,0x09]
# CHECK: cmp r1, r2{{.*}}encoding: [0xf5,0x0a]
# CHECK: cmp r1, r3{{.*}}encoding: [0xf5,0x0b]
# CHECK: cmp r1, r4{{.*}}encoding: [0xf5,0x0c]
# CHECK: cmp r1, r5{{.*}}encoding: [0xf5,0x0d]
# CHECK: cmp r1, r6{{.*}}encoding: [0xf5,0x0e]
# CHECK: cmp r1, r7{{.*}}encoding: [0xf5,0x0f]
# CHECK: cmp r2, r0{{.*}}encoding: [0xf5,0x10]
# CHECK: cmp r2, r1{{.*}}encoding: [0xf5,0x11]
# CHECK: cmp r2, r2{{.*}}encoding: [0xf5,0x12]
# CHECK: cmp r2, r3{{.*}}encoding: [0xf5,0x13]
# CHECK: cmp r2, r4{{.*}}encoding: [0xf5,0x14]
# CHECK: cmp r2, r5{{.*}}encoding: [0xf5,0x15]
# CHECK: cmp r2, r6{{.*}}encoding: [0xf5,0x16]
# CHECK: cmp r2, r7{{.*}}encoding: [0xf5,0x17]
# CHECK: cmp r3, r0{{.*}}encoding: [0xf5,0x18]
# CHECK: cmp r3, r1{{.*}}encoding: [0xf5,0x19]
# CHECK: cmp r3, r2{{.*}}encoding: [0xf5,0x1a]
# CHECK: cmp r3, r3{{.*}}encoding: [0xf5,0x1b]
# CHECK: cmp r3, r4{{.*}}encoding: [0xf5,0x1c]
# CHECK: cmp r3, r5{{.*}}encoding: [0xf5,0x1d]
# CHECK: cmp r3, r6{{.*}}encoding: [0xf5,0x1e]
# CHECK: cmp r3, r7{{.*}}encoding: [0xf5,0x1f]
# CHECK: cmp r4, r0{{.*}}encoding: [0xf5,0x20]
# CHECK: cmp r4, r1{{.*}}encoding: [0xf5,0x21]
# CHECK: cmp r4, r2{{.*}}encoding: [0xf5,0x22]
# CHECK: cmp r4, r3{{.*}}encoding: [0xf5,0x23]
# CHECK: cmp r5, r0{{.*}}encoding: [0xf5,0x24]
# CHECK: cmp r5, r1{{.*}}encoding: [0xf5,0x25]
# CHECK: cmp r5, r2{{.*}}encoding: [0xf5,0x26]
# CHECK: cmp r5, r3{{.*}}encoding: [0xf5,0x27]
# CHECK: cmp r6, r0{{.*}}encoding: [0xf5,0x28]
# CHECK: cmp r6, r1{{.*}}encoding: [0xf5,0x29]
# CHECK: cmp r6, r2{{.*}}encoding: [0xf5,0x2a]
# CHECK: cmp r6, r3{{.*}}encoding: [0xf5,0x2b]
# CHECK: cmp r7, r0{{.*}}encoding: [0xf5,0x2c]
# CHECK: cmp r7, r1{{.*}}encoding: [0xf5,0x2d]
# CHECK: cmp r7, r2{{.*}}encoding: [0xf5,0x2e]
# CHECK: cmp r7, r3{{.*}}encoding: [0xf5,0x2f]

