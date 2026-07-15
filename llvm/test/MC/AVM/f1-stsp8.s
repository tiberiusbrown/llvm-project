# RUN: llvm-mc -triple=avm -show-encoding %s | FileCheck %s

stsp8 [sp+0], c0
stsp8 [sp+0], c1
stsp8 [sp+0], c2
stsp8 [sp+0], c3
stsp8 [sp+1], c0
stsp8 [sp+1], c1
stsp8 [sp+1], c2
stsp8 [sp+1], c3
stsp8 [sp+2], c0
stsp8 [sp+2], c1
stsp8 [sp+2], c2
stsp8 [sp+2], c3
stsp8 [sp+3], c0
stsp8 [sp+3], c1
stsp8 [sp+3], c2
stsp8 [sp+3], c3
stsp8 [sp+4], c0
stsp8 [sp+4], c1
stsp8 [sp+4], c2
stsp8 [sp+4], c3
stsp8 [sp+5], c0
stsp8 [sp+5], c1
stsp8 [sp+5], c2
stsp8 [sp+5], c3
stsp8 [sp+6], c0
stsp8 [sp+6], c1
stsp8 [sp+6], c2
stsp8 [sp+6], c3
stsp8 [sp+7], c0
stsp8 [sp+7], c1
stsp8 [sp+7], c2
stsp8 [sp+7], c3
stsp8 [sp+8], c0
stsp8 [sp+8], c1
stsp8 [sp+8], c2
stsp8 [sp+8], c3
stsp8 [sp+9], c0
stsp8 [sp+9], c1
stsp8 [sp+9], c2
stsp8 [sp+9], c3
stsp8 [sp+10], c0
stsp8 [sp+10], c1
stsp8 [sp+10], c2
stsp8 [sp+10], c3
stsp8 [sp+11], c0
stsp8 [sp+11], c1
stsp8 [sp+11], c2
stsp8 [sp+11], c3
stsp8 [sp+12], c0
stsp8 [sp+12], c1
stsp8 [sp+12], c2
stsp8 [sp+12], c3
stsp8 [sp+13], c0
stsp8 [sp+13], c1
stsp8 [sp+13], c2
stsp8 [sp+13], c3
stsp8 [sp+14], c0
stsp8 [sp+14], c1
stsp8 [sp+14], c2
stsp8 [sp+14], c3
stsp8 [sp+15], c0
stsp8 [sp+15], c1
stsp8 [sp+15], c2
stsp8 [sp+15], c3

# CHECK: encoding: [0xf1,0x30]
# CHECK: encoding: [0xf1,0x31]
# CHECK: encoding: [0xf1,0x32]
# CHECK: encoding: [0xf1,0x33]
# CHECK: encoding: [0xf1,0x34]
# CHECK: encoding: [0xf1,0x35]
# CHECK: encoding: [0xf1,0x36]
# CHECK: encoding: [0xf1,0x37]
# CHECK: encoding: [0xf1,0x38]
# CHECK: encoding: [0xf1,0x39]
# CHECK: encoding: [0xf1,0x3a]
# CHECK: encoding: [0xf1,0x3b]
# CHECK: encoding: [0xf1,0x3c]
# CHECK: encoding: [0xf1,0x3d]
# CHECK: encoding: [0xf1,0x3e]
# CHECK: encoding: [0xf1,0x3f]
# CHECK: encoding: [0xf1,0x40]
# CHECK: encoding: [0xf1,0x41]
# CHECK: encoding: [0xf1,0x42]
# CHECK: encoding: [0xf1,0x43]
# CHECK: encoding: [0xf1,0x44]
# CHECK: encoding: [0xf1,0x45]
# CHECK: encoding: [0xf1,0x46]
# CHECK: encoding: [0xf1,0x47]
# CHECK: encoding: [0xf1,0x48]
# CHECK: encoding: [0xf1,0x49]
# CHECK: encoding: [0xf1,0x4a]
# CHECK: encoding: [0xf1,0x4b]
# CHECK: encoding: [0xf1,0x4c]
# CHECK: encoding: [0xf1,0x4d]
# CHECK: encoding: [0xf1,0x4e]
# CHECK: encoding: [0xf1,0x4f]
# CHECK: encoding: [0xf1,0x50]
# CHECK: encoding: [0xf1,0x51]
# CHECK: encoding: [0xf1,0x52]
# CHECK: encoding: [0xf1,0x53]
# CHECK: encoding: [0xf1,0x54]
# CHECK: encoding: [0xf1,0x55]
# CHECK: encoding: [0xf1,0x56]
# CHECK: encoding: [0xf1,0x57]
# CHECK: encoding: [0xf1,0x58]
# CHECK: encoding: [0xf1,0x59]
# CHECK: encoding: [0xf1,0x5a]
# CHECK: encoding: [0xf1,0x5b]
# CHECK: encoding: [0xf1,0x5c]
# CHECK: encoding: [0xf1,0x5d]
# CHECK: encoding: [0xf1,0x5e]
# CHECK: encoding: [0xf1,0x5f]
# CHECK: encoding: [0xf1,0x60]
# CHECK: encoding: [0xf1,0x61]
# CHECK: encoding: [0xf1,0x62]
# CHECK: encoding: [0xf1,0x63]
# CHECK: encoding: [0xf1,0x64]
# CHECK: encoding: [0xf1,0x65]
# CHECK: encoding: [0xf1,0x66]
# CHECK: encoding: [0xf1,0x67]
# CHECK: encoding: [0xf1,0x68]
# CHECK: encoding: [0xf1,0x69]
# CHECK: encoding: [0xf1,0x6a]
# CHECK: encoding: [0xf1,0x6b]
# CHECK: encoding: [0xf1,0x6c]
# CHECK: encoding: [0xf1,0x6d]
# CHECK: encoding: [0xf1,0x6e]
# CHECK: encoding: [0xf1,0x6f]
