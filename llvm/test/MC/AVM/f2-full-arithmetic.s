# RUN: llvm-mc -triple=avm --show-encoding %s | FileCheck %s

add r0, r0
sub r0, r0
add r0, r1
sub r0, r1
add r0, r2
sub r0, r2
add r0, r3
sub r0, r3
add r0, r4
sub r0, r4
add r0, r5
sub r0, r5
add r0, r6
sub r0, r6
add r0, r7
sub r0, r7
add r1, r0
sub r1, r0
add r1, r1
sub r1, r1
add r1, r2
sub r1, r2
add r1, r3
sub r1, r3
add r1, r4
sub r1, r4
add r1, r5
sub r1, r5
add r1, r6
sub r1, r6
add r1, r7
sub r1, r7
add r2, r0
sub r2, r0
add r2, r1
sub r2, r1
add r2, r2
sub r2, r2
add r2, r3
sub r2, r3
add r2, r4
sub r2, r4
add r2, r5
sub r2, r5
add r2, r6
sub r2, r6
add r2, r7
sub r2, r7
add r3, r0
sub r3, r0
add r3, r1
sub r3, r1
add r3, r2
sub r3, r2
add r3, r3
sub r3, r3
add r3, r4
sub r3, r4
add r3, r5
sub r3, r5
add r3, r6
sub r3, r6
add r3, r7
sub r3, r7
add r4, r0
sub r4, r0
add r4, r1
sub r4, r1
add r4, r2
sub r4, r2
add r4, r3
sub r4, r3
add r5, r0
sub r5, r0
add r5, r1
sub r5, r1
add r5, r2
sub r5, r2
add r5, r3
sub r5, r3
add r6, r0
sub r6, r0
add r6, r1
sub r6, r1
add r6, r2
sub r6, r2
add r6, r3
sub r6, r3
add r7, r0
sub r7, r0
add r7, r1
sub r7, r1
add r7, r2
sub r7, r2
add r7, r3
sub r7, r3

# CHECK: add r0, r0{{.*}}encoding: [0xf2,0x00]
# CHECK: sub r0, r0{{.*}}encoding: [0xf2,0x30]
# CHECK: add r0, r1{{.*}}encoding: [0xf2,0x01]
# CHECK: sub r0, r1{{.*}}encoding: [0xf2,0x31]
# CHECK: add r0, r2{{.*}}encoding: [0xf2,0x02]
# CHECK: sub r0, r2{{.*}}encoding: [0xf2,0x32]
# CHECK: add r0, r3{{.*}}encoding: [0xf2,0x03]
# CHECK: sub r0, r3{{.*}}encoding: [0xf2,0x33]
# CHECK: add r0, r4{{.*}}encoding: [0xf2,0x04]
# CHECK: sub r0, r4{{.*}}encoding: [0xf2,0x34]
# CHECK: add r0, r5{{.*}}encoding: [0xf2,0x05]
# CHECK: sub r0, r5{{.*}}encoding: [0xf2,0x35]
# CHECK: add r0, r6{{.*}}encoding: [0xf2,0x06]
# CHECK: sub r0, r6{{.*}}encoding: [0xf2,0x36]
# CHECK: add r0, r7{{.*}}encoding: [0xf2,0x07]
# CHECK: sub r0, r7{{.*}}encoding: [0xf2,0x37]
# CHECK: add r1, r0{{.*}}encoding: [0xf2,0x08]
# CHECK: sub r1, r0{{.*}}encoding: [0xf2,0x38]
# CHECK: add r1, r1{{.*}}encoding: [0xf2,0x09]
# CHECK: sub r1, r1{{.*}}encoding: [0xf2,0x39]
# CHECK: add r1, r2{{.*}}encoding: [0xf2,0x0a]
# CHECK: sub r1, r2{{.*}}encoding: [0xf2,0x3a]
# CHECK: add r1, r3{{.*}}encoding: [0xf2,0x0b]
# CHECK: sub r1, r3{{.*}}encoding: [0xf2,0x3b]
# CHECK: add r1, r4{{.*}}encoding: [0xf2,0x0c]
# CHECK: sub r1, r4{{.*}}encoding: [0xf2,0x3c]
# CHECK: add r1, r5{{.*}}encoding: [0xf2,0x0d]
# CHECK: sub r1, r5{{.*}}encoding: [0xf2,0x3d]
# CHECK: add r1, r6{{.*}}encoding: [0xf2,0x0e]
# CHECK: sub r1, r6{{.*}}encoding: [0xf2,0x3e]
# CHECK: add r1, r7{{.*}}encoding: [0xf2,0x0f]
# CHECK: sub r1, r7{{.*}}encoding: [0xf2,0x3f]
# CHECK: add r2, r0{{.*}}encoding: [0xf2,0x10]
# CHECK: sub r2, r0{{.*}}encoding: [0xf2,0x40]
# CHECK: add r2, r1{{.*}}encoding: [0xf2,0x11]
# CHECK: sub r2, r1{{.*}}encoding: [0xf2,0x41]
# CHECK: add r2, r2{{.*}}encoding: [0xf2,0x12]
# CHECK: sub r2, r2{{.*}}encoding: [0xf2,0x42]
# CHECK: add r2, r3{{.*}}encoding: [0xf2,0x13]
# CHECK: sub r2, r3{{.*}}encoding: [0xf2,0x43]
# CHECK: add r2, r4{{.*}}encoding: [0xf2,0x14]
# CHECK: sub r2, r4{{.*}}encoding: [0xf2,0x44]
# CHECK: add r2, r5{{.*}}encoding: [0xf2,0x15]
# CHECK: sub r2, r5{{.*}}encoding: [0xf2,0x45]
# CHECK: add r2, r6{{.*}}encoding: [0xf2,0x16]
# CHECK: sub r2, r6{{.*}}encoding: [0xf2,0x46]
# CHECK: add r2, r7{{.*}}encoding: [0xf2,0x17]
# CHECK: sub r2, r7{{.*}}encoding: [0xf2,0x47]
# CHECK: add r3, r0{{.*}}encoding: [0xf2,0x18]
# CHECK: sub r3, r0{{.*}}encoding: [0xf2,0x48]
# CHECK: add r3, r1{{.*}}encoding: [0xf2,0x19]
# CHECK: sub r3, r1{{.*}}encoding: [0xf2,0x49]
# CHECK: add r3, r2{{.*}}encoding: [0xf2,0x1a]
# CHECK: sub r3, r2{{.*}}encoding: [0xf2,0x4a]
# CHECK: add r3, r3{{.*}}encoding: [0xf2,0x1b]
# CHECK: sub r3, r3{{.*}}encoding: [0xf2,0x4b]
# CHECK: add r3, r4{{.*}}encoding: [0xf2,0x1c]
# CHECK: sub r3, r4{{.*}}encoding: [0xf2,0x4c]
# CHECK: add r3, r5{{.*}}encoding: [0xf2,0x1d]
# CHECK: sub r3, r5{{.*}}encoding: [0xf2,0x4d]
# CHECK: add r3, r6{{.*}}encoding: [0xf2,0x1e]
# CHECK: sub r3, r6{{.*}}encoding: [0xf2,0x4e]
# CHECK: add r3, r7{{.*}}encoding: [0xf2,0x1f]
# CHECK: sub r3, r7{{.*}}encoding: [0xf2,0x4f]
# CHECK: add r4, r0{{.*}}encoding: [0xf2,0x20]
# CHECK: sub r4, r0{{.*}}encoding: [0xf2,0x50]
# CHECK: add r4, r1{{.*}}encoding: [0xf2,0x21]
# CHECK: sub r4, r1{{.*}}encoding: [0xf2,0x51]
# CHECK: add r4, r2{{.*}}encoding: [0xf2,0x22]
# CHECK: sub r4, r2{{.*}}encoding: [0xf2,0x52]
# CHECK: add r4, r3{{.*}}encoding: [0xf2,0x23]
# CHECK: sub r4, r3{{.*}}encoding: [0xf2,0x53]
# CHECK: add r5, r0{{.*}}encoding: [0xf2,0x24]
# CHECK: sub r5, r0{{.*}}encoding: [0xf2,0x54]
# CHECK: add r5, r1{{.*}}encoding: [0xf2,0x25]
# CHECK: sub r5, r1{{.*}}encoding: [0xf2,0x55]
# CHECK: add r5, r2{{.*}}encoding: [0xf2,0x26]
# CHECK: sub r5, r2{{.*}}encoding: [0xf2,0x56]
# CHECK: add r5, r3{{.*}}encoding: [0xf2,0x27]
# CHECK: sub r5, r3{{.*}}encoding: [0xf2,0x57]
# CHECK: add r6, r0{{.*}}encoding: [0xf2,0x28]
# CHECK: sub r6, r0{{.*}}encoding: [0xf2,0x58]
# CHECK: add r6, r1{{.*}}encoding: [0xf2,0x29]
# CHECK: sub r6, r1{{.*}}encoding: [0xf2,0x59]
# CHECK: add r6, r2{{.*}}encoding: [0xf2,0x2a]
# CHECK: sub r6, r2{{.*}}encoding: [0xf2,0x5a]
# CHECK: add r6, r3{{.*}}encoding: [0xf2,0x2b]
# CHECK: sub r6, r3{{.*}}encoding: [0xf2,0x5b]
# CHECK: add r7, r0{{.*}}encoding: [0xf2,0x2c]
# CHECK: sub r7, r0{{.*}}encoding: [0xf2,0x5c]
# CHECK: add r7, r1{{.*}}encoding: [0xf2,0x2d]
# CHECK: sub r7, r1{{.*}}encoding: [0xf2,0x5d]
# CHECK: add r7, r2{{.*}}encoding: [0xf2,0x2e]
# CHECK: sub r7, r2{{.*}}encoding: [0xf2,0x5e]
# CHECK: add r7, r3{{.*}}encoding: [0xf2,0x2f]
# CHECK: sub r7, r3{{.*}}encoding: [0xf2,0x5f]

