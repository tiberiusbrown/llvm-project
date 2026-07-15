# RUN: llvm-mc -triple=avm --show-encoding %s | FileCheck %s
# RUN: llvm-mc -triple=avm -filetype=obj %s -o %t.o
# RUN: llvm-objdump -d --no-show-raw-insn %t.o | FileCheck --check-prefix=DIS %s
# CHECK: bswap16 r0{{.*}}encoding: [0xf6,0x20]
# CHECK: bswap16 r1{{.*}}encoding: [0xf6,0x21]
# CHECK: bswap16 r2{{.*}}encoding: [0xf6,0x22]
# CHECK: bswap16 r3{{.*}}encoding: [0xf6,0x23]
# CHECK: bswap16 r4{{.*}}encoding: [0xf6,0x24]
# CHECK: bswap16 r5{{.*}}encoding: [0xf6,0x25]
# CHECK: bswap16 r6{{.*}}encoding: [0xf6,0x26]
# CHECK: bswap16 r7{{.*}}encoding: [0xf6,0x27]
# CHECK: tst16 r0{{.*}}encoding: [0xf6,0x28]
# CHECK: tst16 r1{{.*}}encoding: [0xf6,0x29]
# CHECK: tst16 r2{{.*}}encoding: [0xf6,0x2a]
# CHECK: tst16 r3{{.*}}encoding: [0xf6,0x2b]
# CHECK: tst16 r4{{.*}}encoding: [0xf6,0x2c]
# CHECK: tst16 r5{{.*}}encoding: [0xf6,0x2d]
# CHECK: tst16 r6{{.*}}encoding: [0xf6,0x2e]
# CHECK: tst16 r7{{.*}}encoding: [0xf6,0x2f]
# CHECK: mul8 c0, c0{{.*}}encoding: [0xf6,0x30]
# CHECK: mul8 c0, c1{{.*}}encoding: [0xf6,0x31]
# CHECK: mul8 c0, c2{{.*}}encoding: [0xf6,0x32]
# CHECK: mul8 c0, c3{{.*}}encoding: [0xf6,0x33]
# CHECK: mul8 c1, c0{{.*}}encoding: [0xf6,0x34]
# CHECK: mul8 c1, c1{{.*}}encoding: [0xf6,0x35]
# CHECK: mul8 c1, c2{{.*}}encoding: [0xf6,0x36]
# CHECK: mul8 c1, c3{{.*}}encoding: [0xf6,0x37]
# CHECK: mul8 c2, c0{{.*}}encoding: [0xf6,0x38]
# CHECK: mul8 c2, c1{{.*}}encoding: [0xf6,0x39]
# CHECK: mul8 c2, c2{{.*}}encoding: [0xf6,0x3a]
# CHECK: mul8 c2, c3{{.*}}encoding: [0xf6,0x3b]
# CHECK: mul8 c3, c0{{.*}}encoding: [0xf6,0x3c]
# CHECK: mul8 c3, c1{{.*}}encoding: [0xf6,0x3d]
# CHECK: mul8 c3, c2{{.*}}encoding: [0xf6,0x3e]
# CHECK: mul8 c3, c3{{.*}}encoding: [0xf6,0x3f]
# CHECK: sext8 r0{{.*}}encoding: [0xf6,0x40]
# CHECK: sext8 r1{{.*}}encoding: [0xf6,0x41]
# CHECK: sext8 r2{{.*}}encoding: [0xf6,0x42]
# CHECK: sext8 r3{{.*}}encoding: [0xf6,0x43]
# CHECK: sext8 r4{{.*}}encoding: [0xf6,0x44]
# CHECK: sext8 r5{{.*}}encoding: [0xf6,0x45]
# CHECK: sext8 r6{{.*}}encoding: [0xf6,0x46]
# CHECK: sext8 r7{{.*}}encoding: [0xf6,0x47]
# CHECK: neg16 r0{{.*}}encoding: [0xf6,0x48]
# CHECK: neg16 r1{{.*}}encoding: [0xf6,0x49]
# CHECK: neg16 r2{{.*}}encoding: [0xf6,0x4a]
# CHECK: neg16 r3{{.*}}encoding: [0xf6,0x4b]
# CHECK: neg16 r4{{.*}}encoding: [0xf6,0x4c]
# CHECK: neg16 r5{{.*}}encoding: [0xf6,0x4d]
# CHECK: neg16 r6{{.*}}encoding: [0xf6,0x4e]
# CHECK: neg16 r7{{.*}}encoding: [0xf6,0x4f]
# DIS: bswap16 r0
# DIS: tst16 r7
# DIS: mul8 c0, c0
# DIS: mul8 c3, c3
# DIS: sext8 r0
# DIS: neg16 r7

bswap16 r0
bswap16 r1
bswap16 r2
bswap16 r3
bswap16 r4
bswap16 r5
bswap16 r6
bswap16 r7
tst16 r0
tst16 r1
tst16 r2
tst16 r3
tst16 r4
tst16 r5
tst16 r6
tst16 r7
mul8 c0, c0
mul8 c0, c1
mul8 c0, c2
mul8 c0, c3
mul8 c1, c0
mul8 c1, c1
mul8 c1, c2
mul8 c1, c3
mul8 c2, c0
mul8 c2, c1
mul8 c2, c2
mul8 c2, c3
mul8 c3, c0
mul8 c3, c1
mul8 c3, c2
mul8 c3, c3
sext8 r0
sext8 r1
sext8 r2
sext8 r3
sext8 r4
sext8 r5
sext8 r6
sext8 r7
neg16 r0
neg16 r1
neg16 r2
neg16 r3
neg16 r4
neg16 r5
neg16 r6
neg16 r7
