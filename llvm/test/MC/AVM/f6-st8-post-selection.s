# RUN: llvm-mc -triple=avm -show-encoding %s | FileCheck %s

ST8 [C3+], R7
st8 [r4+], r1
st8 [c0], r1
st8 [c0], c1

# CHECK: st8 [c3+], r7{{.*}}encoding: [0xf6,0x1f]
# CHECK: st8 [r4+], r1{{.*}}encoding: [0xf0,0x6d,0x29]
# CHECK: st8 [c0], r1{{.*}}encoding: [0xf3,0x01]
# CHECK: st8 [c0], c1{{.*}}encoding: [0x51]
