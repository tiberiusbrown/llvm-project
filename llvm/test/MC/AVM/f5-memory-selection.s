# RUN: llvm-mc -triple=avm --show-encoding %s | FileCheck %s

ld8u c0,[c0]
ld8u r0,[c0]
ld16 c3,[c3]
ld16 r3,[c3]
st16 [c1],c2
st16 [c1],r2
LD16 R3,[C0]
St16 [c2],R1

# CHECK: ld8u r4, [r4]{{.*}}encoding: [0x40]
# CHECK: ld8u r0, [r4]{{.*}}encoding: [0xf5,0x30]
# CHECK: ld16 r7, [r7]{{.*}}encoding: [0x6f]
# CHECK: ld16 r3, [r7]{{.*}}encoding: [0xf5,0x4f]
# CHECK: st16 [r5], r6{{.*}}encoding: [0x76]
# CHECK: st16 [r5], r2{{.*}}encoding: [0xf5,0x56]
# CHECK: ld16 r3, [r4]{{.*}}encoding: [0xf5,0x43]
# CHECK: st16 [r6], r1{{.*}}encoding: [0xf5,0x59]
