# RUN: llvm-mc -triple=avm-unknown-arduboyfx -show-encoding %s | FileCheck %s

st8 [c0], r0
st8 [c0], r1
st8 [c0], r2
st8 [c0], r3
st8 [c1], r0
st8 [c1], r1
st8 [c1], r2
st8 [c1], r3
st8 [c2], r0
st8 [c2], r1
st8 [c2], r2
st8 [c2], r3
st8 [c3], r0
st8 [c3], r1
st8 [c3], r2
st8 [c3], r3
mulu8.w c0, c0
mulu8.w c0, c1
mulu8.w c0, c2
mulu8.w c0, c3
mulu8.w c1, c0
mulu8.w c1, c1
mulu8.w c1, c2
mulu8.w c1, c3
mulu8.w c2, c0
mulu8.w c2, c1
mulu8.w c2, c2
mulu8.w c2, c3
mulu8.w c3, c0
mulu8.w c3, c1
mulu8.w c3, c2
mulu8.w c3, c3
muls8.w c0, c0
muls8.w c0, c1
muls8.w c0, c2
muls8.w c0, c3
muls8.w c1, c0
muls8.w c1, c1
muls8.w c1, c2
muls8.w c1, c3
muls8.w c2, c0
muls8.w c2, c1
muls8.w c2, c2
muls8.w c2, c3
muls8.w c3, c0
muls8.w c3, c1
muls8.w c3, c2
muls8.w c3, c3
mulsu8.w c0, c0
mulsu8.w c0, c1
mulsu8.w c0, c2
mulsu8.w c0, c3
mulsu8.w c1, c0
mulsu8.w c1, c1
mulsu8.w c1, c2
mulsu8.w c1, c3
mulsu8.w c2, c0
mulsu8.w c2, c1
mulsu8.w c2, c2
mulsu8.w c2, c3
mulsu8.w c3, c0
mulsu8.w c3, c1
mulsu8.w c3, c2
mulsu8.w c3, c3

# CHECK: encoding: [0xf3,0x00]
# CHECK: encoding: [0xf3,0x01]
# CHECK: encoding: [0xf3,0x02]
# CHECK: encoding: [0xf3,0x03]
# CHECK: encoding: [0xf3,0x04]
# CHECK: encoding: [0xf3,0x05]
# CHECK: encoding: [0xf3,0x06]
# CHECK: encoding: [0xf3,0x07]
# CHECK: encoding: [0xf3,0x08]
# CHECK: encoding: [0xf3,0x09]
# CHECK: encoding: [0xf3,0x0a]
# CHECK: encoding: [0xf3,0x0b]
# CHECK: encoding: [0xf3,0x0c]
# CHECK: encoding: [0xf3,0x0d]
# CHECK: encoding: [0xf3,0x0e]
# CHECK: encoding: [0xf3,0x0f]
# CHECK: encoding: [0xf3,0x10]
# CHECK: encoding: [0xf3,0x11]
# CHECK: encoding: [0xf3,0x12]
# CHECK: encoding: [0xf3,0x13]
# CHECK: encoding: [0xf3,0x14]
# CHECK: encoding: [0xf3,0x15]
# CHECK: encoding: [0xf3,0x16]
# CHECK: encoding: [0xf3,0x17]
# CHECK: encoding: [0xf3,0x18]
# CHECK: encoding: [0xf3,0x19]
# CHECK: encoding: [0xf3,0x1a]
# CHECK: encoding: [0xf3,0x1b]
# CHECK: encoding: [0xf3,0x1c]
# CHECK: encoding: [0xf3,0x1d]
# CHECK: encoding: [0xf3,0x1e]
# CHECK: encoding: [0xf3,0x1f]
# CHECK: encoding: [0xf3,0x20]
# CHECK: encoding: [0xf3,0x21]
# CHECK: encoding: [0xf3,0x22]
# CHECK: encoding: [0xf3,0x23]
# CHECK: encoding: [0xf3,0x24]
# CHECK: encoding: [0xf3,0x25]
# CHECK: encoding: [0xf3,0x26]
# CHECK: encoding: [0xf3,0x27]
# CHECK: encoding: [0xf3,0x28]
# CHECK: encoding: [0xf3,0x29]
# CHECK: encoding: [0xf3,0x2a]
# CHECK: encoding: [0xf3,0x2b]
# CHECK: encoding: [0xf3,0x2c]
# CHECK: encoding: [0xf3,0x2d]
# CHECK: encoding: [0xf3,0x2e]
# CHECK: encoding: [0xf3,0x2f]
# CHECK: encoding: [0xf3,0x30]
# CHECK: encoding: [0xf3,0x31]
# CHECK: encoding: [0xf3,0x32]
# CHECK: encoding: [0xf3,0x33]
# CHECK: encoding: [0xf3,0x34]
# CHECK: encoding: [0xf3,0x35]
# CHECK: encoding: [0xf3,0x36]
# CHECK: encoding: [0xf3,0x37]
# CHECK: encoding: [0xf3,0x38]
# CHECK: encoding: [0xf3,0x39]
# CHECK: encoding: [0xf3,0x3a]
# CHECK: encoding: [0xf3,0x3b]
# CHECK: encoding: [0xf3,0x3c]
# CHECK: encoding: [0xf3,0x3d]
# CHECK: encoding: [0xf3,0x3e]
# CHECK: encoding: [0xf3,0x3f]
