# RUN: llvm-mc -triple=avm-unknown-arduboyfx -show-encoding %s | FileCheck %s

# Every compact-matrix family and every low-nibble operand combination.
mov c0, c0
mov c0, c1
mov c0, c2
mov c0, c3
mov c1, c0
mov c1, c1
mov c1, c2
mov c1, c3
mov c2, c0
mov c2, c1
mov c2, c2
mov c2, c3
mov c3, c0
mov c3, c1
mov c3, c2
mov c3, c3
add c0, c0
add c0, c1
add c0, c2
add c0, c3
add c1, c0
add c1, c1
add c1, c2
add c1, c3
add c2, c0
add c2, c1
add c2, c2
add c2, c3
add c3, c0
add c3, c1
add c3, c2
add c3, c3
sub c0, c0
sub c0, c1
sub c0, c2
sub c0, c3
sub c1, c0
sub c1, c1
sub c1, c2
sub c1, c3
sub c2, c0
sub c2, c1
sub c2, c2
sub c2, c3
sub c3, c0
sub c3, c1
sub c3, c2
sub c3, c3
cmp c0, c0
cmp c0, c1
cmp c0, c2
cmp c0, c3
cmp c1, c0
cmp c1, c1
cmp c1, c2
cmp c1, c3
cmp c2, c0
cmp c2, c1
cmp c2, c2
cmp c2, c3
cmp c3, c0
cmp c3, c1
cmp c3, c2
cmp c3, c3
ld8u c0, [c0]
ld8u c0, [c1]
ld8u c0, [c2]
ld8u c0, [c3]
ld8u c1, [c0]
ld8u c1, [c1]
ld8u c1, [c2]
ld8u c1, [c3]
ld8u c2, [c0]
ld8u c2, [c1]
ld8u c2, [c2]
ld8u c2, [c3]
ld8u c3, [c0]
ld8u c3, [c1]
ld8u c3, [c2]
ld8u c3, [c3]
st8 [c0], c0
st8 [c0], c1
st8 [c0], c2
st8 [c0], c3
st8 [c1], c0
st8 [c1], c1
st8 [c1], c2
st8 [c1], c3
st8 [c2], c0
st8 [c2], c1
st8 [c2], c2
st8 [c2], c3
st8 [c3], c0
st8 [c3], c1
st8 [c3], c2
st8 [c3], c3
ld16 c0, [c0]
ld16 c0, [c1]
ld16 c0, [c2]
ld16 c0, [c3]
ld16 c1, [c0]
ld16 c1, [c1]
ld16 c1, [c2]
ld16 c1, [c3]
ld16 c2, [c0]
ld16 c2, [c1]
ld16 c2, [c2]
ld16 c2, [c3]
ld16 c3, [c0]
ld16 c3, [c1]
ld16 c3, [c2]
ld16 c3, [c3]
st16 [c0], c0
st16 [c0], c1
st16 [c0], c2
st16 [c0], c3
st16 [c1], c0
st16 [c1], c1
st16 [c1], c2
st16 [c1], c3
st16 [c2], c0
st16 [c2], c1
st16 [c2], c2
st16 [c2], c3
st16 [c3], c0
st16 [c3], c1
st16 [c3], c2
st16 [c3], c3
and c0, c0
and c0, c1
and c0, c2
and c0, c3
and c1, c0
and c1, c1
and c1, c2
and c1, c3
and c2, c0
and c2, c1
and c2, c2
and c2, c3
and c3, c0
and c3, c1
and c3, c2
and c3, c3
or c0, c0
or c0, c1
or c0, c2
or c0, c3
or c1, c0
or c1, c1
or c1, c2
or c1, c3
or c2, c0
or c2, c1
or c2, c2
or c2, c3
or c3, c0
or c3, c1
or c3, c2
or c3, c3
xor c0, c0
xor c0, c1
xor c0, c2
xor c0, c3
xor c1, c0
xor c1, c1
xor c1, c2
xor c1, c3
xor c2, c0
xor c2, c1
xor c2, c2
xor c2, c3
xor c3, c0
xor c3, c1
xor c3, c2
xor c3, c3

# CHECK: nop{{.*}}encoding: [0x00]
# CHECK: mov{{.*}}r4, r5{{.*}}encoding: [0x01]
# CHECK: mov{{.*}}r4, r6{{.*}}encoding: [0x02]
# CHECK: mov{{.*}}r4, r7{{.*}}encoding: [0x03]
# CHECK: mov{{.*}}r5, r4{{.*}}encoding: [0x04]
# CHECK: mov{{.*}}r5, r5{{.*}}encoding: [0x05]
# CHECK: mov{{.*}}r5, r6{{.*}}encoding: [0x06]
# CHECK: mov{{.*}}r5, r7{{.*}}encoding: [0x07]
# CHECK: mov{{.*}}r6, r4{{.*}}encoding: [0x08]
# CHECK: mov{{.*}}r6, r5{{.*}}encoding: [0x09]
# CHECK: mov{{.*}}r6, r6{{.*}}encoding: [0x0a]
# CHECK: mov{{.*}}r6, r7{{.*}}encoding: [0x0b]
# CHECK: mov{{.*}}r7, r4{{.*}}encoding: [0x0c]
# CHECK: mov{{.*}}r7, r5{{.*}}encoding: [0x0d]
# CHECK: mov{{.*}}r7, r6{{.*}}encoding: [0x0e]
# CHECK: mov{{.*}}r7, r7{{.*}}encoding: [0x0f]
# CHECK: add{{.*}}r4, r4{{.*}}encoding: [0x10]
# CHECK: add{{.*}}r4, r5{{.*}}encoding: [0x11]
# CHECK: add{{.*}}r4, r6{{.*}}encoding: [0x12]
# CHECK: add{{.*}}r4, r7{{.*}}encoding: [0x13]
# CHECK: add{{.*}}r5, r4{{.*}}encoding: [0x14]
# CHECK: add{{.*}}r5, r5{{.*}}encoding: [0x15]
# CHECK: add{{.*}}r5, r6{{.*}}encoding: [0x16]
# CHECK: add{{.*}}r5, r7{{.*}}encoding: [0x17]
# CHECK: add{{.*}}r6, r4{{.*}}encoding: [0x18]
# CHECK: add{{.*}}r6, r5{{.*}}encoding: [0x19]
# CHECK: add{{.*}}r6, r6{{.*}}encoding: [0x1a]
# CHECK: add{{.*}}r6, r7{{.*}}encoding: [0x1b]
# CHECK: add{{.*}}r7, r4{{.*}}encoding: [0x1c]
# CHECK: add{{.*}}r7, r5{{.*}}encoding: [0x1d]
# CHECK: add{{.*}}r7, r6{{.*}}encoding: [0x1e]
# CHECK: add{{.*}}r7, r7{{.*}}encoding: [0x1f]
# CHECK: sub{{.*}}r4, r4{{.*}}encoding: [0x20]
# CHECK: sub{{.*}}r4, r5{{.*}}encoding: [0x21]
# CHECK: sub{{.*}}r4, r6{{.*}}encoding: [0x22]
# CHECK: sub{{.*}}r4, r7{{.*}}encoding: [0x23]
# CHECK: sub{{.*}}r5, r4{{.*}}encoding: [0x24]
# CHECK: sub{{.*}}r5, r5{{.*}}encoding: [0x25]
# CHECK: sub{{.*}}r5, r6{{.*}}encoding: [0x26]
# CHECK: sub{{.*}}r5, r7{{.*}}encoding: [0x27]
# CHECK: sub{{.*}}r6, r4{{.*}}encoding: [0x28]
# CHECK: sub{{.*}}r6, r5{{.*}}encoding: [0x29]
# CHECK: sub{{.*}}r6, r6{{.*}}encoding: [0x2a]
# CHECK: sub{{.*}}r6, r7{{.*}}encoding: [0x2b]
# CHECK: sub{{.*}}r7, r4{{.*}}encoding: [0x2c]
# CHECK: sub{{.*}}r7, r5{{.*}}encoding: [0x2d]
# CHECK: sub{{.*}}r7, r6{{.*}}encoding: [0x2e]
# CHECK: sub{{.*}}r7, r7{{.*}}encoding: [0x2f]
# CHECK: cmp{{.*}}r4, r4{{.*}}encoding: [0x30]
# CHECK: cmp{{.*}}r4, r5{{.*}}encoding: [0x31]
# CHECK: cmp{{.*}}r4, r6{{.*}}encoding: [0x32]
# CHECK: cmp{{.*}}r4, r7{{.*}}encoding: [0x33]
# CHECK: cmp{{.*}}r5, r4{{.*}}encoding: [0x34]
# CHECK: cmp{{.*}}r5, r5{{.*}}encoding: [0x35]
# CHECK: cmp{{.*}}r5, r6{{.*}}encoding: [0x36]
# CHECK: cmp{{.*}}r5, r7{{.*}}encoding: [0x37]
# CHECK: cmp{{.*}}r6, r4{{.*}}encoding: [0x38]
# CHECK: cmp{{.*}}r6, r5{{.*}}encoding: [0x39]
# CHECK: cmp{{.*}}r6, r6{{.*}}encoding: [0x3a]
# CHECK: cmp{{.*}}r6, r7{{.*}}encoding: [0x3b]
# CHECK: cmp{{.*}}r7, r4{{.*}}encoding: [0x3c]
# CHECK: cmp{{.*}}r7, r5{{.*}}encoding: [0x3d]
# CHECK: cmp{{.*}}r7, r6{{.*}}encoding: [0x3e]
# CHECK: cmp{{.*}}r7, r7{{.*}}encoding: [0x3f]
# CHECK: ld8u{{.*}}r4, [r4]{{.*}}encoding: [0x40]
# CHECK: ld8u{{.*}}r4, [r5]{{.*}}encoding: [0x41]
# CHECK: ld8u{{.*}}r4, [r6]{{.*}}encoding: [0x42]
# CHECK: ld8u{{.*}}r4, [r7]{{.*}}encoding: [0x43]
# CHECK: ld8u{{.*}}r5, [r4]{{.*}}encoding: [0x44]
# CHECK: ld8u{{.*}}r5, [r5]{{.*}}encoding: [0x45]
# CHECK: ld8u{{.*}}r5, [r6]{{.*}}encoding: [0x46]
# CHECK: ld8u{{.*}}r5, [r7]{{.*}}encoding: [0x47]
# CHECK: ld8u{{.*}}r6, [r4]{{.*}}encoding: [0x48]
# CHECK: ld8u{{.*}}r6, [r5]{{.*}}encoding: [0x49]
# CHECK: ld8u{{.*}}r6, [r6]{{.*}}encoding: [0x4a]
# CHECK: ld8u{{.*}}r6, [r7]{{.*}}encoding: [0x4b]
# CHECK: ld8u{{.*}}r7, [r4]{{.*}}encoding: [0x4c]
# CHECK: ld8u{{.*}}r7, [r5]{{.*}}encoding: [0x4d]
# CHECK: ld8u{{.*}}r7, [r6]{{.*}}encoding: [0x4e]
# CHECK: ld8u{{.*}}r7, [r7]{{.*}}encoding: [0x4f]
# CHECK: st8{{.*}}[r4], r4{{.*}}encoding: [0x50]
# CHECK: st8{{.*}}[r4], r5{{.*}}encoding: [0x51]
# CHECK: st8{{.*}}[r4], r6{{.*}}encoding: [0x52]
# CHECK: st8{{.*}}[r4], r7{{.*}}encoding: [0x53]
# CHECK: st8{{.*}}[r5], r4{{.*}}encoding: [0x54]
# CHECK: st8{{.*}}[r5], r5{{.*}}encoding: [0x55]
# CHECK: st8{{.*}}[r5], r6{{.*}}encoding: [0x56]
# CHECK: st8{{.*}}[r5], r7{{.*}}encoding: [0x57]
# CHECK: st8{{.*}}[r6], r4{{.*}}encoding: [0x58]
# CHECK: st8{{.*}}[r6], r5{{.*}}encoding: [0x59]
# CHECK: st8{{.*}}[r6], r6{{.*}}encoding: [0x5a]
# CHECK: st8{{.*}}[r6], r7{{.*}}encoding: [0x5b]
# CHECK: st8{{.*}}[r7], r4{{.*}}encoding: [0x5c]
# CHECK: st8{{.*}}[r7], r5{{.*}}encoding: [0x5d]
# CHECK: st8{{.*}}[r7], r6{{.*}}encoding: [0x5e]
# CHECK: st8{{.*}}[r7], r7{{.*}}encoding: [0x5f]
# CHECK: ld16{{.*}}r4, [r4]{{.*}}encoding: [0x60]
# CHECK: ld16{{.*}}r4, [r5]{{.*}}encoding: [0x61]
# CHECK: ld16{{.*}}r4, [r6]{{.*}}encoding: [0x62]
# CHECK: ld16{{.*}}r4, [r7]{{.*}}encoding: [0x63]
# CHECK: ld16{{.*}}r5, [r4]{{.*}}encoding: [0x64]
# CHECK: ld16{{.*}}r5, [r5]{{.*}}encoding: [0x65]
# CHECK: ld16{{.*}}r5, [r6]{{.*}}encoding: [0x66]
# CHECK: ld16{{.*}}r5, [r7]{{.*}}encoding: [0x67]
# CHECK: ld16{{.*}}r6, [r4]{{.*}}encoding: [0x68]
# CHECK: ld16{{.*}}r6, [r5]{{.*}}encoding: [0x69]
# CHECK: ld16{{.*}}r6, [r6]{{.*}}encoding: [0x6a]
# CHECK: ld16{{.*}}r6, [r7]{{.*}}encoding: [0x6b]
# CHECK: ld16{{.*}}r7, [r4]{{.*}}encoding: [0x6c]
# CHECK: ld16{{.*}}r7, [r5]{{.*}}encoding: [0x6d]
# CHECK: ld16{{.*}}r7, [r6]{{.*}}encoding: [0x6e]
# CHECK: ld16{{.*}}r7, [r7]{{.*}}encoding: [0x6f]
# CHECK: st16{{.*}}[r4], r4{{.*}}encoding: [0x70]
# CHECK: st16{{.*}}[r4], r5{{.*}}encoding: [0x71]
# CHECK: st16{{.*}}[r4], r6{{.*}}encoding: [0x72]
# CHECK: st16{{.*}}[r4], r7{{.*}}encoding: [0x73]
# CHECK: st16{{.*}}[r5], r4{{.*}}encoding: [0x74]
# CHECK: st16{{.*}}[r5], r5{{.*}}encoding: [0x75]
# CHECK: st16{{.*}}[r5], r6{{.*}}encoding: [0x76]
# CHECK: st16{{.*}}[r5], r7{{.*}}encoding: [0x77]
# CHECK: st16{{.*}}[r6], r4{{.*}}encoding: [0x78]
# CHECK: st16{{.*}}[r6], r5{{.*}}encoding: [0x79]
# CHECK: st16{{.*}}[r6], r6{{.*}}encoding: [0x7a]
# CHECK: st16{{.*}}[r6], r7{{.*}}encoding: [0x7b]
# CHECK: st16{{.*}}[r7], r4{{.*}}encoding: [0x7c]
# CHECK: st16{{.*}}[r7], r5{{.*}}encoding: [0x7d]
# CHECK: st16{{.*}}[r7], r6{{.*}}encoding: [0x7e]
# CHECK: st16{{.*}}[r7], r7{{.*}}encoding: [0x7f]
# CHECK: and{{.*}}r4, r4{{.*}}encoding: [0x80]
# CHECK: and{{.*}}r4, r5{{.*}}encoding: [0x81]
# CHECK: and{{.*}}r4, r6{{.*}}encoding: [0x82]
# CHECK: and{{.*}}r4, r7{{.*}}encoding: [0x83]
# CHECK: and{{.*}}r5, r4{{.*}}encoding: [0x84]
# CHECK: and{{.*}}r5, r5{{.*}}encoding: [0x85]
# CHECK: and{{.*}}r5, r6{{.*}}encoding: [0x86]
# CHECK: and{{.*}}r5, r7{{.*}}encoding: [0x87]
# CHECK: and{{.*}}r6, r4{{.*}}encoding: [0x88]
# CHECK: and{{.*}}r6, r5{{.*}}encoding: [0x89]
# CHECK: and{{.*}}r6, r6{{.*}}encoding: [0x8a]
# CHECK: and{{.*}}r6, r7{{.*}}encoding: [0x8b]
# CHECK: and{{.*}}r7, r4{{.*}}encoding: [0x8c]
# CHECK: and{{.*}}r7, r5{{.*}}encoding: [0x8d]
# CHECK: and{{.*}}r7, r6{{.*}}encoding: [0x8e]
# CHECK: and{{.*}}r7, r7{{.*}}encoding: [0x8f]
# CHECK: or{{.*}}r4, r4{{.*}}encoding: [0x90]
# CHECK: or{{.*}}r4, r5{{.*}}encoding: [0x91]
# CHECK: or{{.*}}r4, r6{{.*}}encoding: [0x92]
# CHECK: or{{.*}}r4, r7{{.*}}encoding: [0x93]
# CHECK: or{{.*}}r5, r4{{.*}}encoding: [0x94]
# CHECK: or{{.*}}r5, r5{{.*}}encoding: [0x95]
# CHECK: or{{.*}}r5, r6{{.*}}encoding: [0x96]
# CHECK: or{{.*}}r5, r7{{.*}}encoding: [0x97]
# CHECK: or{{.*}}r6, r4{{.*}}encoding: [0x98]
# CHECK: or{{.*}}r6, r5{{.*}}encoding: [0x99]
# CHECK: or{{.*}}r6, r6{{.*}}encoding: [0x9a]
# CHECK: or{{.*}}r6, r7{{.*}}encoding: [0x9b]
# CHECK: or{{.*}}r7, r4{{.*}}encoding: [0x9c]
# CHECK: or{{.*}}r7, r5{{.*}}encoding: [0x9d]
# CHECK: or{{.*}}r7, r6{{.*}}encoding: [0x9e]
# CHECK: or{{.*}}r7, r7{{.*}}encoding: [0x9f]
# CHECK: xor{{.*}}r4, r4{{.*}}encoding: [0xa0]
# CHECK: xor{{.*}}r4, r5{{.*}}encoding: [0xa1]
# CHECK: xor{{.*}}r4, r6{{.*}}encoding: [0xa2]
# CHECK: xor{{.*}}r4, r7{{.*}}encoding: [0xa3]
# CHECK: xor{{.*}}r5, r4{{.*}}encoding: [0xa4]
# CHECK: xor{{.*}}r5, r5{{.*}}encoding: [0xa5]
# CHECK: xor{{.*}}r5, r6{{.*}}encoding: [0xa6]
# CHECK: xor{{.*}}r5, r7{{.*}}encoding: [0xa7]
# CHECK: xor{{.*}}r6, r4{{.*}}encoding: [0xa8]
# CHECK: xor{{.*}}r6, r5{{.*}}encoding: [0xa9]
# CHECK: xor{{.*}}r6, r6{{.*}}encoding: [0xaa]
# CHECK: xor{{.*}}r6, r7{{.*}}encoding: [0xab]
# CHECK: xor{{.*}}r7, r4{{.*}}encoding: [0xac]
# CHECK: xor{{.*}}r7, r5{{.*}}encoding: [0xad]
# CHECK: xor{{.*}}r7, r6{{.*}}encoding: [0xae]
# CHECK: xor{{.*}}r7, r7{{.*}}encoding: [0xaf]
