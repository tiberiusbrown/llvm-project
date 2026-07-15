# RUN: llvm-mc -triple=avm -show-encoding %s | FileCheck %s

mov r0, r0
mov r0, r1
mov r0, r2
mov r0, r3
mov r0, r4
mov r0, r5
mov r0, r6
mov r0, r7
mov r1, r0
mov r1, r1
mov r1, r2
mov r1, r3
mov r1, r4
mov r1, r5
mov r1, r6
mov r1, r7
mov r2, r0
mov r2, r1
mov r2, r2
mov r2, r3
mov r2, r4
mov r2, r5
mov r2, r6
mov r2, r7
mov r3, r0
mov r3, r1
mov r3, r2
mov r3, r3
mov r3, r4
mov r3, r5
mov r3, r6
mov r3, r7
mov r4, r0
mov r4, r1
mov r4, r2
mov r4, r3
mov r5, r0
mov r5, r1
mov r5, r2
mov r5, r3
mov r6, r0
mov r6, r1
mov r6, r2
mov r6, r3
mov r7, r0
mov r7, r1
mov r7, r2
mov r7, r3

# CHECK: mov{{[ \t]+}}r0, r0{{.*}}encoding: [0xf1,0x00]
# CHECK: mov{{[ \t]+}}r0, r1{{.*}}encoding: [0xf1,0x01]
# CHECK: mov{{[ \t]+}}r0, r2{{.*}}encoding: [0xf1,0x02]
# CHECK: mov{{[ \t]+}}r0, r3{{.*}}encoding: [0xf1,0x03]
# CHECK: mov{{[ \t]+}}r0, r4{{.*}}encoding: [0xf1,0x04]
# CHECK: mov{{[ \t]+}}r0, r5{{.*}}encoding: [0xf1,0x05]
# CHECK: mov{{[ \t]+}}r0, r6{{.*}}encoding: [0xf1,0x06]
# CHECK: mov{{[ \t]+}}r0, r7{{.*}}encoding: [0xf1,0x07]
# CHECK: mov{{[ \t]+}}r1, r0{{.*}}encoding: [0xf1,0x08]
# CHECK: mov{{[ \t]+}}r1, r1{{.*}}encoding: [0xf1,0x09]
# CHECK: mov{{[ \t]+}}r1, r2{{.*}}encoding: [0xf1,0x0a]
# CHECK: mov{{[ \t]+}}r1, r3{{.*}}encoding: [0xf1,0x0b]
# CHECK: mov{{[ \t]+}}r1, r4{{.*}}encoding: [0xf1,0x0c]
# CHECK: mov{{[ \t]+}}r1, r5{{.*}}encoding: [0xf1,0x0d]
# CHECK: mov{{[ \t]+}}r1, r6{{.*}}encoding: [0xf1,0x0e]
# CHECK: mov{{[ \t]+}}r1, r7{{.*}}encoding: [0xf1,0x0f]
# CHECK: mov{{[ \t]+}}r2, r0{{.*}}encoding: [0xf1,0x10]
# CHECK: mov{{[ \t]+}}r2, r1{{.*}}encoding: [0xf1,0x11]
# CHECK: mov{{[ \t]+}}r2, r2{{.*}}encoding: [0xf1,0x12]
# CHECK: mov{{[ \t]+}}r2, r3{{.*}}encoding: [0xf1,0x13]
# CHECK: mov{{[ \t]+}}r2, r4{{.*}}encoding: [0xf1,0x14]
# CHECK: mov{{[ \t]+}}r2, r5{{.*}}encoding: [0xf1,0x15]
# CHECK: mov{{[ \t]+}}r2, r6{{.*}}encoding: [0xf1,0x16]
# CHECK: mov{{[ \t]+}}r2, r7{{.*}}encoding: [0xf1,0x17]
# CHECK: mov{{[ \t]+}}r3, r0{{.*}}encoding: [0xf1,0x18]
# CHECK: mov{{[ \t]+}}r3, r1{{.*}}encoding: [0xf1,0x19]
# CHECK: mov{{[ \t]+}}r3, r2{{.*}}encoding: [0xf1,0x1a]
# CHECK: mov{{[ \t]+}}r3, r3{{.*}}encoding: [0xf1,0x1b]
# CHECK: mov{{[ \t]+}}r3, r4{{.*}}encoding: [0xf1,0x1c]
# CHECK: mov{{[ \t]+}}r3, r5{{.*}}encoding: [0xf1,0x1d]
# CHECK: mov{{[ \t]+}}r3, r6{{.*}}encoding: [0xf1,0x1e]
# CHECK: mov{{[ \t]+}}r3, r7{{.*}}encoding: [0xf1,0x1f]
# CHECK: mov{{[ \t]+}}r4, r0{{.*}}encoding: [0xf1,0x20]
# CHECK: mov{{[ \t]+}}r4, r1{{.*}}encoding: [0xf1,0x21]
# CHECK: mov{{[ \t]+}}r4, r2{{.*}}encoding: [0xf1,0x22]
# CHECK: mov{{[ \t]+}}r4, r3{{.*}}encoding: [0xf1,0x23]
# CHECK: mov{{[ \t]+}}r5, r0{{.*}}encoding: [0xf1,0x24]
# CHECK: mov{{[ \t]+}}r5, r1{{.*}}encoding: [0xf1,0x25]
# CHECK: mov{{[ \t]+}}r5, r2{{.*}}encoding: [0xf1,0x26]
# CHECK: mov{{[ \t]+}}r5, r3{{.*}}encoding: [0xf1,0x27]
# CHECK: mov{{[ \t]+}}r6, r0{{.*}}encoding: [0xf1,0x28]
# CHECK: mov{{[ \t]+}}r6, r1{{.*}}encoding: [0xf1,0x29]
# CHECK: mov{{[ \t]+}}r6, r2{{.*}}encoding: [0xf1,0x2a]
# CHECK: mov{{[ \t]+}}r6, r3{{.*}}encoding: [0xf1,0x2b]
# CHECK: mov{{[ \t]+}}r7, r0{{.*}}encoding: [0xf1,0x2c]
# CHECK: mov{{[ \t]+}}r7, r1{{.*}}encoding: [0xf1,0x2d]
# CHECK: mov{{[ \t]+}}r7, r2{{.*}}encoding: [0xf1,0x2e]
# CHECK: mov{{[ \t]+}}r7, r3{{.*}}encoding: [0xf1,0x2f]
