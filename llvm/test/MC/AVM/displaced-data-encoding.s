# RUN: llvm-mc -triple=avm -show-encoding %s | FileCheck %s

ld8u r0, [r0-32]
ld8u r3, [r5+0]
ld16 r4, [r2-1]
ld16 r7, [r7+223]
st8 [r7-32], r0
st8 [r5+0], r3
st16 [r2-1], r4
st16 [r0+223], r7

# Explicit zero displacement must not select an ordinary-memory encoding.
ld8u r4, [r5]
ld8u r0, [r5]
ld8u r0, [r1]
ld8u r0, [r1+]
ld8u r0, [r1+0]

# Data/address overlap and compact aliases are legal for displaced forms.
ld8u r3, [r3+0]
ld16 r6, [r6-1]
st8 [r4+7], r4
st16 [r7-32], r7
ld8u c0, [c1+0]
st8 [c0+1], r0
st8 [c0 + 1], r0

# CHECK: encoding: [0xed,0x00,0x00]
# CHECK: encoding: [0xed,0x6a,0x20]
# CHECK: encoding: [0xed,0x94,0x1f]
# CHECK: encoding: [0xed,0xfe,0xff]
# CHECK: encoding: [0xee,0x0e,0x00]
# CHECK: encoding: [0xee,0x6a,0x20]
# CHECK: encoding: [0xee,0x94,0x1f]
# CHECK: encoding: [0xee,0xf0,0xff]
# CHECK: encoding: [0x41]
# CHECK: encoding: [0xf5,0x34]
# CHECK: encoding: [0xf0,0x6c,0x02]
# CHECK: encoding: [0xf0,0x6c,0x03]
# CHECK: encoding: [0xed,0x02,0x20]
# CHECK: encoding: [0xed,0x66,0x20]
# CHECK: encoding: [0xed,0xdc,0x1f]
# CHECK: encoding: [0xee,0x88,0x27]
# CHECK: encoding: [0xee,0xfe,0x00]
# CHECK: encoding: [0xed,0x8a,0x20]
# CHECK: encoding: [0xee,0x08,0x21]
# CHECK: encoding: [0xee,0x08,0x21]
