# RUN: llvm-mc -triple=avm -show-encoding %s | FileCheck %s

# Canonical cN spellings retain their compact matrix encodings, while the
# identical physical registers spelled rN select the F0 general-pointer forms.
ld8u c0, [c1]
ld8u r4, [r5]
ld16 c2, [c3]
ld16 r6, [r7]
st8 [c0], c1
st8 [r4], r5
st16 [c2], c3
st16 [r6], r7

# Case-insensitive input still prints canonically in lowercase.
LD8U R0, [R7]
ST16 [R7+], R7

# CHECK: ld8u{{.*}}c0, [c1]{{.*}}encoding: [0x41]
# CHECK: ld8u{{.*}}r4, [r5]{{.*}}encoding: [0xf0,0x6c,0x8a]
# CHECK: ld16{{.*}}c2, [c3]{{.*}}encoding: [0x6b]
# CHECK: ld16{{.*}}r6, [r7]{{.*}}encoding: [0xf0,0x6c,0xde]
# CHECK: st8{{.*}}[c0], c1{{.*}}encoding: [0x51]
# CHECK: st8{{.*}}[r4], r5{{.*}}encoding: [0xf0,0x6d,0xa8]
# CHECK: st16{{.*}}[c2], c3{{.*}}encoding: [0x7b]
# CHECK: st16{{.*}}[r6], r7{{.*}}encoding: [0xf0,0x6d,0xfc]
# CHECK: ld8u{{.*}}r0, [r7]{{.*}}encoding: [0xf0,0x6c,0x0e]
# CHECK: st16{{.*}}[r7+], r7{{.*}}encoding: [0xf0,0x6d,0xff]
