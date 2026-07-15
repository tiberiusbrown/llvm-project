# RUN: llvm-mc -triple=avm -show-encoding %s | FileCheck %s

st8 [c0+], r0
st8 [c0+], r1
st8 [c0+], r2
st8 [c0+], r3
st8 [c0+], r4
st8 [c0+], r5
st8 [c0+], r6
st8 [c0+], r7
st8 [c1+], r0
st8 [c1+], r1
st8 [c1+], r2
st8 [c1+], r3
st8 [c1+], r4
st8 [c1+], r5
st8 [c1+], r6
st8 [c1+], r7
st8 [c2+], r0
st8 [c2+], r1
st8 [c2+], r2
st8 [c2+], r3
st8 [c2+], r4
st8 [c2+], r5
st8 [c2+], r6
st8 [c2+], r7
st8 [c3+], r0
st8 [c3+], r1
st8 [c3+], r2
st8 [c3+], r3
st8 [c3+], r4
st8 [c3+], r5
st8 [c3+], r6
st8 [c3+], r7

# CHECK: st8 [c0+], r0{{.*}}encoding: [0xf6,0x00]
# CHECK: st8 [c0+], r1{{.*}}encoding: [0xf6,0x01]
# CHECK: st8 [c0+], r2{{.*}}encoding: [0xf6,0x02]
# CHECK: st8 [c0+], r3{{.*}}encoding: [0xf6,0x03]
# CHECK: st8 [c0+], r4{{.*}}encoding: [0xf6,0x04]
# CHECK: st8 [c0+], r5{{.*}}encoding: [0xf6,0x05]
# CHECK: st8 [c0+], r6{{.*}}encoding: [0xf6,0x06]
# CHECK: st8 [c0+], r7{{.*}}encoding: [0xf6,0x07]
# CHECK: st8 [c1+], r0{{.*}}encoding: [0xf6,0x08]
# CHECK: st8 [c1+], r1{{.*}}encoding: [0xf6,0x09]
# CHECK: st8 [c1+], r2{{.*}}encoding: [0xf6,0x0a]
# CHECK: st8 [c1+], r3{{.*}}encoding: [0xf6,0x0b]
# CHECK: st8 [c1+], r4{{.*}}encoding: [0xf6,0x0c]
# CHECK: st8 [c1+], r5{{.*}}encoding: [0xf6,0x0d]
# CHECK: st8 [c1+], r6{{.*}}encoding: [0xf6,0x0e]
# CHECK: st8 [c1+], r7{{.*}}encoding: [0xf6,0x0f]
# CHECK: st8 [c2+], r0{{.*}}encoding: [0xf6,0x10]
# CHECK: st8 [c2+], r1{{.*}}encoding: [0xf6,0x11]
# CHECK: st8 [c2+], r2{{.*}}encoding: [0xf6,0x12]
# CHECK: st8 [c2+], r3{{.*}}encoding: [0xf6,0x13]
# CHECK: st8 [c2+], r4{{.*}}encoding: [0xf6,0x14]
# CHECK: st8 [c2+], r5{{.*}}encoding: [0xf6,0x15]
# CHECK: st8 [c2+], r6{{.*}}encoding: [0xf6,0x16]
# CHECK: st8 [c2+], r7{{.*}}encoding: [0xf6,0x17]
# CHECK: st8 [c3+], r0{{.*}}encoding: [0xf6,0x18]
# CHECK: st8 [c3+], r1{{.*}}encoding: [0xf6,0x19]
# CHECK: st8 [c3+], r2{{.*}}encoding: [0xf6,0x1a]
# CHECK: st8 [c3+], r3{{.*}}encoding: [0xf6,0x1b]
# CHECK: st8 [c3+], r4{{.*}}encoding: [0xf6,0x1c]
# CHECK: st8 [c3+], r5{{.*}}encoding: [0xf6,0x1d]
# CHECK: st8 [c3+], r6{{.*}}encoding: [0xf6,0x1e]
# CHECK: st8 [c3+], r7{{.*}}encoding: [0xf6,0x1f]
