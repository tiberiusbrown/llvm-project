# RUN: llvm-mc -triple=avm-unknown-arduboyfx -show-encoding %s | FileCheck %s
# RUN: llvm-mc -triple=avm-unknown-arduboyfx -filetype=obj %s -o %t.o
# RUN: llvm-objdump -d --no-show-raw-insn %t.o | FileCheck %s --check-prefix=ROUNDTRIP

ldsp16 c0, [sp+0]
ldsp16 c1, [sp+0]
ldsp16 c2, [sp+0]
ldsp16 c3, [sp+0]
ldsp16 c0, [sp+1]
ldsp16 c1, [sp+1]
ldsp16 c2, [sp+1]
ldsp16 c3, [sp+1]
ldsp16 c0, [sp+2]
ldsp16 c1, [sp+2]
ldsp16 c2, [sp+2]
ldsp16 c3, [sp+2]
ldsp16 c0, [sp+3]
ldsp16 c1, [sp+3]
ldsp16 c2, [sp+3]
ldsp16 c3, [sp+3]
ldsp16 c0, [sp+4]
ldsp16 c1, [sp+4]
ldsp16 c2, [sp+4]
ldsp16 c3, [sp+4]
ldsp16 c0, [sp+5]
ldsp16 c1, [sp+5]
ldsp16 c2, [sp+5]
ldsp16 c3, [sp+5]
ldsp16 c0, [sp+6]
ldsp16 c1, [sp+6]
ldsp16 c2, [sp+6]
ldsp16 c3, [sp+6]
ldsp16 c0, [sp+7]
ldsp16 c1, [sp+7]
ldsp16 c2, [sp+7]
ldsp16 c3, [sp+7]
ldsp16 c0, [sp+8]
ldsp16 c1, [sp+8]
ldsp16 c2, [sp+8]
ldsp16 c3, [sp+8]
ldsp16 c0, [sp+9]
ldsp16 c1, [sp+9]
ldsp16 c2, [sp+9]
ldsp16 c3, [sp+9]
ldsp16 c0, [sp+10]
ldsp16 c1, [sp+10]
ldsp16 c2, [sp+10]
ldsp16 c3, [sp+10]
ldsp16 c0, [sp+11]
ldsp16 c1, [sp+11]
ldsp16 c2, [sp+11]
ldsp16 c3, [sp+11]
ldsp16 c0, [sp+12]
ldsp16 c1, [sp+12]
ldsp16 c2, [sp+12]
ldsp16 c3, [sp+12]
ldsp16 c0, [sp+13]
ldsp16 c1, [sp+13]
ldsp16 c2, [sp+13]
ldsp16 c3, [sp+13]
ldsp16 c0, [sp+14]
ldsp16 c1, [sp+14]
ldsp16 c2, [sp+14]
ldsp16 c3, [sp+14]
ldsp16 c0, [sp+15]
ldsp16 c1, [sp+15]
ldsp16 c2, [sp+15]
ldsp16 c3, [sp+15]
stsp16 [sp+0], c0
stsp16 [sp+0], c1
stsp16 [sp+0], c2
stsp16 [sp+0], c3
stsp16 [sp+1], c0
stsp16 [sp+1], c1
stsp16 [sp+1], c2
stsp16 [sp+1], c3
stsp16 [sp+2], c0
stsp16 [sp+2], c1
stsp16 [sp+2], c2
stsp16 [sp+2], c3
stsp16 [sp+3], c0
stsp16 [sp+3], c1
stsp16 [sp+3], c2
stsp16 [sp+3], c3
stsp16 [sp+4], c0
stsp16 [sp+4], c1
stsp16 [sp+4], c2
stsp16 [sp+4], c3
stsp16 [sp+5], c0
stsp16 [sp+5], c1
stsp16 [sp+5], c2
stsp16 [sp+5], c3
stsp16 [sp+6], c0
stsp16 [sp+6], c1
stsp16 [sp+6], c2
stsp16 [sp+6], c3
stsp16 [sp+7], c0
stsp16 [sp+7], c1
stsp16 [sp+7], c2
stsp16 [sp+7], c3
stsp16 [sp+8], c0
stsp16 [sp+8], c1
stsp16 [sp+8], c2
stsp16 [sp+8], c3
stsp16 [sp+9], c0
stsp16 [sp+9], c1
stsp16 [sp+9], c2
stsp16 [sp+9], c3
stsp16 [sp+10], c0
stsp16 [sp+10], c1
stsp16 [sp+10], c2
stsp16 [sp+10], c3
stsp16 [sp+11], c0
stsp16 [sp+11], c1
stsp16 [sp+11], c2
stsp16 [sp+11], c3
stsp16 [sp+12], c0
stsp16 [sp+12], c1
stsp16 [sp+12], c2
stsp16 [sp+12], c3
stsp16 [sp+13], c0
stsp16 [sp+13], c1
stsp16 [sp+13], c2
stsp16 [sp+13], c3
stsp16 [sp+14], c0
stsp16 [sp+14], c1
stsp16 [sp+14], c2
stsp16 [sp+14], c3
stsp16 [sp+15], c0
stsp16 [sp+15], c1
stsp16 [sp+15], c2
stsp16 [sp+15], c3

# CHECK: encoding: [0xf4,0x00]
# CHECK: encoding: [0xf4,0x01]
# CHECK: encoding: [0xf4,0x02]
# CHECK: encoding: [0xf4,0x03]
# CHECK: encoding: [0xf4,0x04]
# CHECK: encoding: [0xf4,0x05]
# CHECK: encoding: [0xf4,0x06]
# CHECK: encoding: [0xf4,0x07]
# CHECK: encoding: [0xf4,0x08]
# CHECK: encoding: [0xf4,0x09]
# CHECK: encoding: [0xf4,0x0a]
# CHECK: encoding: [0xf4,0x0b]
# CHECK: encoding: [0xf4,0x0c]
# CHECK: encoding: [0xf4,0x0d]
# CHECK: encoding: [0xf4,0x0e]
# CHECK: encoding: [0xf4,0x0f]
# CHECK: encoding: [0xf4,0x10]
# CHECK: encoding: [0xf4,0x11]
# CHECK: encoding: [0xf4,0x12]
# CHECK: encoding: [0xf4,0x13]
# CHECK: encoding: [0xf4,0x14]
# CHECK: encoding: [0xf4,0x15]
# CHECK: encoding: [0xf4,0x16]
# CHECK: encoding: [0xf4,0x17]
# CHECK: encoding: [0xf4,0x18]
# CHECK: encoding: [0xf4,0x19]
# CHECK: encoding: [0xf4,0x1a]
# CHECK: encoding: [0xf4,0x1b]
# CHECK: encoding: [0xf4,0x1c]
# CHECK: encoding: [0xf4,0x1d]
# CHECK: encoding: [0xf4,0x1e]
# CHECK: encoding: [0xf4,0x1f]
# CHECK: encoding: [0xf4,0x20]
# CHECK: encoding: [0xf4,0x21]
# CHECK: encoding: [0xf4,0x22]
# CHECK: encoding: [0xf4,0x23]
# CHECK: encoding: [0xf4,0x24]
# CHECK: encoding: [0xf4,0x25]
# CHECK: encoding: [0xf4,0x26]
# CHECK: encoding: [0xf4,0x27]
# CHECK: encoding: [0xf4,0x28]
# CHECK: encoding: [0xf4,0x29]
# CHECK: encoding: [0xf4,0x2a]
# CHECK: encoding: [0xf4,0x2b]
# CHECK: encoding: [0xf4,0x2c]
# CHECK: encoding: [0xf4,0x2d]
# CHECK: encoding: [0xf4,0x2e]
# CHECK: encoding: [0xf4,0x2f]
# CHECK: encoding: [0xf4,0x30]
# CHECK: encoding: [0xf4,0x31]
# CHECK: encoding: [0xf4,0x32]
# CHECK: encoding: [0xf4,0x33]
# CHECK: encoding: [0xf4,0x34]
# CHECK: encoding: [0xf4,0x35]
# CHECK: encoding: [0xf4,0x36]
# CHECK: encoding: [0xf4,0x37]
# CHECK: encoding: [0xf4,0x38]
# CHECK: encoding: [0xf4,0x39]
# CHECK: encoding: [0xf4,0x3a]
# CHECK: encoding: [0xf4,0x3b]
# CHECK: encoding: [0xf4,0x3c]
# CHECK: encoding: [0xf4,0x3d]
# CHECK: encoding: [0xf4,0x3e]
# CHECK: encoding: [0xf4,0x3f]
# CHECK: encoding: [0xf4,0x40]
# CHECK: encoding: [0xf4,0x41]
# CHECK: encoding: [0xf4,0x42]
# CHECK: encoding: [0xf4,0x43]
# CHECK: encoding: [0xf4,0x44]
# CHECK: encoding: [0xf4,0x45]
# CHECK: encoding: [0xf4,0x46]
# CHECK: encoding: [0xf4,0x47]
# CHECK: encoding: [0xf4,0x48]
# CHECK: encoding: [0xf4,0x49]
# CHECK: encoding: [0xf4,0x4a]
# CHECK: encoding: [0xf4,0x4b]
# CHECK: encoding: [0xf4,0x4c]
# CHECK: encoding: [0xf4,0x4d]
# CHECK: encoding: [0xf4,0x4e]
# CHECK: encoding: [0xf4,0x4f]
# CHECK: encoding: [0xf4,0x50]
# CHECK: encoding: [0xf4,0x51]
# CHECK: encoding: [0xf4,0x52]
# CHECK: encoding: [0xf4,0x53]
# CHECK: encoding: [0xf4,0x54]
# CHECK: encoding: [0xf4,0x55]
# CHECK: encoding: [0xf4,0x56]
# CHECK: encoding: [0xf4,0x57]
# CHECK: encoding: [0xf4,0x58]
# CHECK: encoding: [0xf4,0x59]
# CHECK: encoding: [0xf4,0x5a]
# CHECK: encoding: [0xf4,0x5b]
# CHECK: encoding: [0xf4,0x5c]
# CHECK: encoding: [0xf4,0x5d]
# CHECK: encoding: [0xf4,0x5e]
# CHECK: encoding: [0xf4,0x5f]
# CHECK: encoding: [0xf4,0x60]
# CHECK: encoding: [0xf4,0x61]
# CHECK: encoding: [0xf4,0x62]
# CHECK: encoding: [0xf4,0x63]
# CHECK: encoding: [0xf4,0x64]
# CHECK: encoding: [0xf4,0x65]
# CHECK: encoding: [0xf4,0x66]
# CHECK: encoding: [0xf4,0x67]
# CHECK: encoding: [0xf4,0x68]
# CHECK: encoding: [0xf4,0x69]
# CHECK: encoding: [0xf4,0x6a]
# CHECK: encoding: [0xf4,0x6b]
# CHECK: encoding: [0xf4,0x6c]
# CHECK: encoding: [0xf4,0x6d]
# CHECK: encoding: [0xf4,0x6e]
# CHECK: encoding: [0xf4,0x6f]
# CHECK: encoding: [0xf4,0x70]
# CHECK: encoding: [0xf4,0x71]
# CHECK: encoding: [0xf4,0x72]
# CHECK: encoding: [0xf4,0x73]
# CHECK: encoding: [0xf4,0x74]
# CHECK: encoding: [0xf4,0x75]
# CHECK: encoding: [0xf4,0x76]
# CHECK: encoding: [0xf4,0x77]
# CHECK: encoding: [0xf4,0x78]
# CHECK: encoding: [0xf4,0x79]
# CHECK: encoding: [0xf4,0x7a]
# CHECK: encoding: [0xf4,0x7b]
# CHECK: encoding: [0xf4,0x7c]
# CHECK: encoding: [0xf4,0x7d]
# CHECK: encoding: [0xf4,0x7e]
# CHECK: encoding: [0xf4,0x7f]
# ROUNDTRIP: ldsp16 r4, [sp+0x0]
# ROUNDTRIP: ldsp16 r5, [sp+0x0]
# ROUNDTRIP: ldsp16 r6, [sp+0x0]
# ROUNDTRIP: ldsp16 r7, [sp+0x0]
# ROUNDTRIP: ldsp16 r4, [sp+0x1]
# ROUNDTRIP: ldsp16 r5, [sp+0x1]
# ROUNDTRIP: ldsp16 r6, [sp+0x1]
# ROUNDTRIP: ldsp16 r7, [sp+0x1]
# ROUNDTRIP: ldsp16 r4, [sp+0x2]
# ROUNDTRIP: ldsp16 r5, [sp+0x2]
# ROUNDTRIP: ldsp16 r6, [sp+0x2]
# ROUNDTRIP: ldsp16 r7, [sp+0x2]
# ROUNDTRIP: ldsp16 r4, [sp+0x3]
# ROUNDTRIP: ldsp16 r5, [sp+0x3]
# ROUNDTRIP: ldsp16 r6, [sp+0x3]
# ROUNDTRIP: ldsp16 r7, [sp+0x3]
# ROUNDTRIP: ldsp16 r4, [sp+0x4]
# ROUNDTRIP: ldsp16 r5, [sp+0x4]
# ROUNDTRIP: ldsp16 r6, [sp+0x4]
# ROUNDTRIP: ldsp16 r7, [sp+0x4]
# ROUNDTRIP: ldsp16 r4, [sp+0x5]
# ROUNDTRIP: ldsp16 r5, [sp+0x5]
# ROUNDTRIP: ldsp16 r6, [sp+0x5]
# ROUNDTRIP: ldsp16 r7, [sp+0x5]
# ROUNDTRIP: ldsp16 r4, [sp+0x6]
# ROUNDTRIP: ldsp16 r5, [sp+0x6]
# ROUNDTRIP: ldsp16 r6, [sp+0x6]
# ROUNDTRIP: ldsp16 r7, [sp+0x6]
# ROUNDTRIP: ldsp16 r4, [sp+0x7]
# ROUNDTRIP: ldsp16 r5, [sp+0x7]
# ROUNDTRIP: ldsp16 r6, [sp+0x7]
# ROUNDTRIP: ldsp16 r7, [sp+0x7]
# ROUNDTRIP: ldsp16 r4, [sp+0x8]
# ROUNDTRIP: ldsp16 r5, [sp+0x8]
# ROUNDTRIP: ldsp16 r6, [sp+0x8]
# ROUNDTRIP: ldsp16 r7, [sp+0x8]
# ROUNDTRIP: ldsp16 r4, [sp+0x9]
# ROUNDTRIP: ldsp16 r5, [sp+0x9]
# ROUNDTRIP: ldsp16 r6, [sp+0x9]
# ROUNDTRIP: ldsp16 r7, [sp+0x9]
# ROUNDTRIP: ldsp16 r4, [sp+0xa]
# ROUNDTRIP: ldsp16 r5, [sp+0xa]
# ROUNDTRIP: ldsp16 r6, [sp+0xa]
# ROUNDTRIP: ldsp16 r7, [sp+0xa]
# ROUNDTRIP: ldsp16 r4, [sp+0xb]
# ROUNDTRIP: ldsp16 r5, [sp+0xb]
# ROUNDTRIP: ldsp16 r6, [sp+0xb]
# ROUNDTRIP: ldsp16 r7, [sp+0xb]
# ROUNDTRIP: ldsp16 r4, [sp+0xc]
# ROUNDTRIP: ldsp16 r5, [sp+0xc]
# ROUNDTRIP: ldsp16 r6, [sp+0xc]
# ROUNDTRIP: ldsp16 r7, [sp+0xc]
# ROUNDTRIP: ldsp16 r4, [sp+0xd]
# ROUNDTRIP: ldsp16 r5, [sp+0xd]
# ROUNDTRIP: ldsp16 r6, [sp+0xd]
# ROUNDTRIP: ldsp16 r7, [sp+0xd]
# ROUNDTRIP: ldsp16 r4, [sp+0xe]
# ROUNDTRIP: ldsp16 r5, [sp+0xe]
# ROUNDTRIP: ldsp16 r6, [sp+0xe]
# ROUNDTRIP: ldsp16 r7, [sp+0xe]
# ROUNDTRIP: ldsp16 r4, [sp+0xf]
# ROUNDTRIP: ldsp16 r5, [sp+0xf]
# ROUNDTRIP: ldsp16 r6, [sp+0xf]
# ROUNDTRIP: ldsp16 r7, [sp+0xf]
# ROUNDTRIP: stsp16 [sp+0x0], r4
# ROUNDTRIP: stsp16 [sp+0x0], r5
# ROUNDTRIP: stsp16 [sp+0x0], r6
# ROUNDTRIP: stsp16 [sp+0x0], r7
# ROUNDTRIP: stsp16 [sp+0x1], r4
# ROUNDTRIP: stsp16 [sp+0x1], r5
# ROUNDTRIP: stsp16 [sp+0x1], r6
# ROUNDTRIP: stsp16 [sp+0x1], r7
# ROUNDTRIP: stsp16 [sp+0x2], r4
# ROUNDTRIP: stsp16 [sp+0x2], r5
# ROUNDTRIP: stsp16 [sp+0x2], r6
# ROUNDTRIP: stsp16 [sp+0x2], r7
# ROUNDTRIP: stsp16 [sp+0x3], r4
# ROUNDTRIP: stsp16 [sp+0x3], r5
# ROUNDTRIP: stsp16 [sp+0x3], r6
# ROUNDTRIP: stsp16 [sp+0x3], r7
# ROUNDTRIP: stsp16 [sp+0x4], r4
# ROUNDTRIP: stsp16 [sp+0x4], r5
# ROUNDTRIP: stsp16 [sp+0x4], r6
# ROUNDTRIP: stsp16 [sp+0x4], r7
# ROUNDTRIP: stsp16 [sp+0x5], r4
# ROUNDTRIP: stsp16 [sp+0x5], r5
# ROUNDTRIP: stsp16 [sp+0x5], r6
# ROUNDTRIP: stsp16 [sp+0x5], r7
# ROUNDTRIP: stsp16 [sp+0x6], r4
# ROUNDTRIP: stsp16 [sp+0x6], r5
# ROUNDTRIP: stsp16 [sp+0x6], r6
# ROUNDTRIP: stsp16 [sp+0x6], r7
# ROUNDTRIP: stsp16 [sp+0x7], r4
# ROUNDTRIP: stsp16 [sp+0x7], r5
# ROUNDTRIP: stsp16 [sp+0x7], r6
# ROUNDTRIP: stsp16 [sp+0x7], r7
# ROUNDTRIP: stsp16 [sp+0x8], r4
# ROUNDTRIP: stsp16 [sp+0x8], r5
# ROUNDTRIP: stsp16 [sp+0x8], r6
# ROUNDTRIP: stsp16 [sp+0x8], r7
# ROUNDTRIP: stsp16 [sp+0x9], r4
# ROUNDTRIP: stsp16 [sp+0x9], r5
# ROUNDTRIP: stsp16 [sp+0x9], r6
# ROUNDTRIP: stsp16 [sp+0x9], r7
# ROUNDTRIP: stsp16 [sp+0xa], r4
# ROUNDTRIP: stsp16 [sp+0xa], r5
# ROUNDTRIP: stsp16 [sp+0xa], r6
# ROUNDTRIP: stsp16 [sp+0xa], r7
# ROUNDTRIP: stsp16 [sp+0xb], r4
# ROUNDTRIP: stsp16 [sp+0xb], r5
# ROUNDTRIP: stsp16 [sp+0xb], r6
# ROUNDTRIP: stsp16 [sp+0xb], r7
# ROUNDTRIP: stsp16 [sp+0xc], r4
# ROUNDTRIP: stsp16 [sp+0xc], r5
# ROUNDTRIP: stsp16 [sp+0xc], r6
# ROUNDTRIP: stsp16 [sp+0xc], r7
# ROUNDTRIP: stsp16 [sp+0xd], r4
# ROUNDTRIP: stsp16 [sp+0xd], r5
# ROUNDTRIP: stsp16 [sp+0xd], r6
# ROUNDTRIP: stsp16 [sp+0xd], r7
# ROUNDTRIP: stsp16 [sp+0xe], r4
# ROUNDTRIP: stsp16 [sp+0xe], r5
# ROUNDTRIP: stsp16 [sp+0xe], r6
# ROUNDTRIP: stsp16 [sp+0xe], r7
# ROUNDTRIP: stsp16 [sp+0xf], r4
# ROUNDTRIP: stsp16 [sp+0xf], r5
# ROUNDTRIP: stsp16 [sp+0xf], r6
# ROUNDTRIP: stsp16 [sp+0xf], r7
