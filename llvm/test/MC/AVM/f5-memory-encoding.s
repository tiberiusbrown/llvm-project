# RUN: llvm-mc -triple=avm --show-encoding %s | FileCheck %s

ld8u r0, [c0]
ld8u r1, [c0]
ld8u r2, [c0]
ld8u r3, [c0]
ld8u r0, [c1]
ld8u r1, [c1]
ld8u r2, [c1]
ld8u r3, [c1]
ld8u r0, [c2]
ld8u r1, [c2]
ld8u r2, [c2]
ld8u r3, [c2]
ld8u r0, [c3]
ld8u r1, [c3]
ld8u r2, [c3]
ld8u r3, [c3]
ld16 r0, [c0]
ld16 r1, [c0]
ld16 r2, [c0]
ld16 r3, [c0]
ld16 r0, [c1]
ld16 r1, [c1]
ld16 r2, [c1]
ld16 r3, [c1]
ld16 r0, [c2]
ld16 r1, [c2]
ld16 r2, [c2]
ld16 r3, [c2]
ld16 r0, [c3]
ld16 r1, [c3]
ld16 r2, [c3]
ld16 r3, [c3]
st16 [c0], r0
st16 [c0], r1
st16 [c0], r2
st16 [c0], r3
st16 [c1], r0
st16 [c1], r1
st16 [c1], r2
st16 [c1], r3
st16 [c2], r0
st16 [c2], r1
st16 [c2], r2
st16 [c2], r3
st16 [c3], r0
st16 [c3], r1
st16 [c3], r2
st16 [c3], r3

# CHECK: ld8u r0, [r4]{{.*}}encoding: [0xf5,0x30]
# CHECK: ld8u r1, [r4]{{.*}}encoding: [0xf5,0x31]
# CHECK: ld8u r2, [r4]{{.*}}encoding: [0xf5,0x32]
# CHECK: ld8u r3, [r4]{{.*}}encoding: [0xf5,0x33]
# CHECK: ld8u r0, [r5]{{.*}}encoding: [0xf5,0x34]
# CHECK: ld8u r1, [r5]{{.*}}encoding: [0xf5,0x35]
# CHECK: ld8u r2, [r5]{{.*}}encoding: [0xf5,0x36]
# CHECK: ld8u r3, [r5]{{.*}}encoding: [0xf5,0x37]
# CHECK: ld8u r0, [r6]{{.*}}encoding: [0xf5,0x38]
# CHECK: ld8u r1, [r6]{{.*}}encoding: [0xf5,0x39]
# CHECK: ld8u r2, [r6]{{.*}}encoding: [0xf5,0x3a]
# CHECK: ld8u r3, [r6]{{.*}}encoding: [0xf5,0x3b]
# CHECK: ld8u r0, [r7]{{.*}}encoding: [0xf5,0x3c]
# CHECK: ld8u r1, [r7]{{.*}}encoding: [0xf5,0x3d]
# CHECK: ld8u r2, [r7]{{.*}}encoding: [0xf5,0x3e]
# CHECK: ld8u r3, [r7]{{.*}}encoding: [0xf5,0x3f]
# CHECK: ld16 r0, [r4]{{.*}}encoding: [0xf5,0x40]
# CHECK: ld16 r1, [r4]{{.*}}encoding: [0xf5,0x41]
# CHECK: ld16 r2, [r4]{{.*}}encoding: [0xf5,0x42]
# CHECK: ld16 r3, [r4]{{.*}}encoding: [0xf5,0x43]
# CHECK: ld16 r0, [r5]{{.*}}encoding: [0xf5,0x44]
# CHECK: ld16 r1, [r5]{{.*}}encoding: [0xf5,0x45]
# CHECK: ld16 r2, [r5]{{.*}}encoding: [0xf5,0x46]
# CHECK: ld16 r3, [r5]{{.*}}encoding: [0xf5,0x47]
# CHECK: ld16 r0, [r6]{{.*}}encoding: [0xf5,0x48]
# CHECK: ld16 r1, [r6]{{.*}}encoding: [0xf5,0x49]
# CHECK: ld16 r2, [r6]{{.*}}encoding: [0xf5,0x4a]
# CHECK: ld16 r3, [r6]{{.*}}encoding: [0xf5,0x4b]
# CHECK: ld16 r0, [r7]{{.*}}encoding: [0xf5,0x4c]
# CHECK: ld16 r1, [r7]{{.*}}encoding: [0xf5,0x4d]
# CHECK: ld16 r2, [r7]{{.*}}encoding: [0xf5,0x4e]
# CHECK: ld16 r3, [r7]{{.*}}encoding: [0xf5,0x4f]
# CHECK: st16 [r4], r0{{.*}}encoding: [0xf5,0x50]
# CHECK: st16 [r4], r1{{.*}}encoding: [0xf5,0x51]
# CHECK: st16 [r4], r2{{.*}}encoding: [0xf5,0x52]
# CHECK: st16 [r4], r3{{.*}}encoding: [0xf5,0x53]
# CHECK: st16 [r5], r0{{.*}}encoding: [0xf5,0x54]
# CHECK: st16 [r5], r1{{.*}}encoding: [0xf5,0x55]
# CHECK: st16 [r5], r2{{.*}}encoding: [0xf5,0x56]
# CHECK: st16 [r5], r3{{.*}}encoding: [0xf5,0x57]
# CHECK: st16 [r6], r0{{.*}}encoding: [0xf5,0x58]
# CHECK: st16 [r6], r1{{.*}}encoding: [0xf5,0x59]
# CHECK: st16 [r6], r2{{.*}}encoding: [0xf5,0x5a]
# CHECK: st16 [r6], r3{{.*}}encoding: [0xf5,0x5b]
# CHECK: st16 [r7], r0{{.*}}encoding: [0xf5,0x5c]
# CHECK: st16 [r7], r1{{.*}}encoding: [0xf5,0x5d]
# CHECK: st16 [r7], r2{{.*}}encoding: [0xf5,0x5e]
# CHECK: st16 [r7], r3{{.*}}encoding: [0xf5,0x5f]
