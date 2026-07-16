# RUN: llvm-mc -triple=avm -show-encoding < %s | FileCheck %s

cmov.eq r0, r0
# CHECK: cmov.eq r0, r0{{.*}}encoding: [0xfb,0x00]
cmov.eq r0, r1
# CHECK: cmov.eq r0, r1{{.*}}encoding: [0xfb,0x01]
cmov.eq r0, r2
# CHECK: cmov.eq r0, r2{{.*}}encoding: [0xfb,0x02]
cmov.eq r0, r3
# CHECK: cmov.eq r0, r3{{.*}}encoding: [0xfb,0x03]
cmov.eq r0, r4
# CHECK: cmov.eq r0, r4{{.*}}encoding: [0xfb,0x04]
cmov.eq r0, r5
# CHECK: cmov.eq r0, r5{{.*}}encoding: [0xfb,0x05]
cmov.eq r0, r6
# CHECK: cmov.eq r0, r6{{.*}}encoding: [0xfb,0x06]
cmov.eq r0, r7
# CHECK: cmov.eq r0, r7{{.*}}encoding: [0xfb,0x07]
cmov.eq r1, r0
# CHECK: cmov.eq r1, r0{{.*}}encoding: [0xfb,0x08]
cmov.eq r1, r1
# CHECK: cmov.eq r1, r1{{.*}}encoding: [0xfb,0x09]
cmov.eq r1, r2
# CHECK: cmov.eq r1, r2{{.*}}encoding: [0xfb,0x0a]
cmov.eq r1, r3
# CHECK: cmov.eq r1, r3{{.*}}encoding: [0xfb,0x0b]
cmov.eq r1, r4
# CHECK: cmov.eq r1, r4{{.*}}encoding: [0xfb,0x0c]
cmov.eq r1, r5
# CHECK: cmov.eq r1, r5{{.*}}encoding: [0xfb,0x0d]
cmov.eq r1, r6
# CHECK: cmov.eq r1, r6{{.*}}encoding: [0xfb,0x0e]
cmov.eq r1, r7
# CHECK: cmov.eq r1, r7{{.*}}encoding: [0xfb,0x0f]
cmov.eq r2, r0
# CHECK: cmov.eq r2, r0{{.*}}encoding: [0xfb,0x10]
cmov.eq r2, r1
# CHECK: cmov.eq r2, r1{{.*}}encoding: [0xfb,0x11]
cmov.eq r2, r2
# CHECK: cmov.eq r2, r2{{.*}}encoding: [0xfb,0x12]
cmov.eq r2, r3
# CHECK: cmov.eq r2, r3{{.*}}encoding: [0xfb,0x13]
cmov.eq r2, r4
# CHECK: cmov.eq r2, r4{{.*}}encoding: [0xfb,0x14]
cmov.eq r2, r5
# CHECK: cmov.eq r2, r5{{.*}}encoding: [0xfb,0x15]
cmov.eq r2, r6
# CHECK: cmov.eq r2, r6{{.*}}encoding: [0xfb,0x16]
cmov.eq r2, r7
# CHECK: cmov.eq r2, r7{{.*}}encoding: [0xfb,0x17]
cmov.eq r3, r0
# CHECK: cmov.eq r3, r0{{.*}}encoding: [0xfb,0x18]
cmov.eq r3, r1
# CHECK: cmov.eq r3, r1{{.*}}encoding: [0xfb,0x19]
cmov.eq r3, r2
# CHECK: cmov.eq r3, r2{{.*}}encoding: [0xfb,0x1a]
cmov.eq r3, r3
# CHECK: cmov.eq r3, r3{{.*}}encoding: [0xfb,0x1b]
cmov.eq r3, r4
# CHECK: cmov.eq r3, r4{{.*}}encoding: [0xfb,0x1c]
cmov.eq r3, r5
# CHECK: cmov.eq r3, r5{{.*}}encoding: [0xfb,0x1d]
cmov.eq r3, r6
# CHECK: cmov.eq r3, r6{{.*}}encoding: [0xfb,0x1e]
cmov.eq r3, r7
# CHECK: cmov.eq r3, r7{{.*}}encoding: [0xfb,0x1f]
cmov.eq r4, r0
# CHECK: cmov.eq r4, r0{{.*}}encoding: [0xfb,0x20]
cmov.eq r4, r1
# CHECK: cmov.eq r4, r1{{.*}}encoding: [0xfb,0x21]
cmov.eq r4, r2
# CHECK: cmov.eq r4, r2{{.*}}encoding: [0xfb,0x22]
cmov.eq r4, r3
# CHECK: cmov.eq r4, r3{{.*}}encoding: [0xfb,0x23]
cmov.eq r4, r4
# CHECK: cmov.eq r4, r4{{.*}}encoding: [0xfb,0x24]
cmov.eq r4, r5
# CHECK: cmov.eq r4, r5{{.*}}encoding: [0xfb,0x25]
cmov.eq r4, r6
# CHECK: cmov.eq r4, r6{{.*}}encoding: [0xfb,0x26]
cmov.eq r4, r7
# CHECK: cmov.eq r4, r7{{.*}}encoding: [0xfb,0x27]
cmov.eq r5, r0
# CHECK: cmov.eq r5, r0{{.*}}encoding: [0xfb,0x28]
cmov.eq r5, r1
# CHECK: cmov.eq r5, r1{{.*}}encoding: [0xfb,0x29]
cmov.eq r5, r2
# CHECK: cmov.eq r5, r2{{.*}}encoding: [0xfb,0x2a]
cmov.eq r5, r3
# CHECK: cmov.eq r5, r3{{.*}}encoding: [0xfb,0x2b]
cmov.eq r5, r4
# CHECK: cmov.eq r5, r4{{.*}}encoding: [0xfb,0x2c]
cmov.eq r5, r5
# CHECK: cmov.eq r5, r5{{.*}}encoding: [0xfb,0x2d]
cmov.eq r5, r6
# CHECK: cmov.eq r5, r6{{.*}}encoding: [0xfb,0x2e]
cmov.eq r5, r7
# CHECK: cmov.eq r5, r7{{.*}}encoding: [0xfb,0x2f]
cmov.eq r6, r0
# CHECK: cmov.eq r6, r0{{.*}}encoding: [0xfb,0x30]
cmov.eq r6, r1
# CHECK: cmov.eq r6, r1{{.*}}encoding: [0xfb,0x31]
cmov.eq r6, r2
# CHECK: cmov.eq r6, r2{{.*}}encoding: [0xfb,0x32]
cmov.eq r6, r3
# CHECK: cmov.eq r6, r3{{.*}}encoding: [0xfb,0x33]
cmov.eq r6, r4
# CHECK: cmov.eq r6, r4{{.*}}encoding: [0xfb,0x34]
cmov.eq r6, r5
# CHECK: cmov.eq r6, r5{{.*}}encoding: [0xfb,0x35]
cmov.eq r6, r6
# CHECK: cmov.eq r6, r6{{.*}}encoding: [0xfb,0x36]
cmov.eq r6, r7
# CHECK: cmov.eq r6, r7{{.*}}encoding: [0xfb,0x37]
cmov.eq r7, r0
# CHECK: cmov.eq r7, r0{{.*}}encoding: [0xfb,0x38]
cmov.eq r7, r1
# CHECK: cmov.eq r7, r1{{.*}}encoding: [0xfb,0x39]
cmov.eq r7, r2
# CHECK: cmov.eq r7, r2{{.*}}encoding: [0xfb,0x3a]
cmov.eq r7, r3
# CHECK: cmov.eq r7, r3{{.*}}encoding: [0xfb,0x3b]
cmov.eq r7, r4
# CHECK: cmov.eq r7, r4{{.*}}encoding: [0xfb,0x3c]
cmov.eq r7, r5
# CHECK: cmov.eq r7, r5{{.*}}encoding: [0xfb,0x3d]
cmov.eq r7, r6
# CHECK: cmov.eq r7, r6{{.*}}encoding: [0xfb,0x3e]
cmov.eq r7, r7
# CHECK: cmov.eq r7, r7{{.*}}encoding: [0xfb,0x3f]
cmov.ne r0, r0
# CHECK: cmov.ne r0, r0{{.*}}encoding: [0xfb,0x40]
cmov.ne r0, r1
# CHECK: cmov.ne r0, r1{{.*}}encoding: [0xfb,0x41]
cmov.ne r0, r2
# CHECK: cmov.ne r0, r2{{.*}}encoding: [0xfb,0x42]
cmov.ne r0, r3
# CHECK: cmov.ne r0, r3{{.*}}encoding: [0xfb,0x43]
cmov.ne r0, r4
# CHECK: cmov.ne r0, r4{{.*}}encoding: [0xfb,0x44]
cmov.ne r0, r5
# CHECK: cmov.ne r0, r5{{.*}}encoding: [0xfb,0x45]
cmov.ne r0, r6
# CHECK: cmov.ne r0, r6{{.*}}encoding: [0xfb,0x46]
cmov.ne r0, r7
# CHECK: cmov.ne r0, r7{{.*}}encoding: [0xfb,0x47]
cmov.ne r1, r0
# CHECK: cmov.ne r1, r0{{.*}}encoding: [0xfb,0x48]
cmov.ne r1, r1
# CHECK: cmov.ne r1, r1{{.*}}encoding: [0xfb,0x49]
cmov.ne r1, r2
# CHECK: cmov.ne r1, r2{{.*}}encoding: [0xfb,0x4a]
cmov.ne r1, r3
# CHECK: cmov.ne r1, r3{{.*}}encoding: [0xfb,0x4b]
cmov.ne r1, r4
# CHECK: cmov.ne r1, r4{{.*}}encoding: [0xfb,0x4c]
cmov.ne r1, r5
# CHECK: cmov.ne r1, r5{{.*}}encoding: [0xfb,0x4d]
cmov.ne r1, r6
# CHECK: cmov.ne r1, r6{{.*}}encoding: [0xfb,0x4e]
cmov.ne r1, r7
# CHECK: cmov.ne r1, r7{{.*}}encoding: [0xfb,0x4f]
cmov.ne r2, r0
# CHECK: cmov.ne r2, r0{{.*}}encoding: [0xfb,0x50]
cmov.ne r2, r1
# CHECK: cmov.ne r2, r1{{.*}}encoding: [0xfb,0x51]
cmov.ne r2, r2
# CHECK: cmov.ne r2, r2{{.*}}encoding: [0xfb,0x52]
cmov.ne r2, r3
# CHECK: cmov.ne r2, r3{{.*}}encoding: [0xfb,0x53]
cmov.ne r2, r4
# CHECK: cmov.ne r2, r4{{.*}}encoding: [0xfb,0x54]
cmov.ne r2, r5
# CHECK: cmov.ne r2, r5{{.*}}encoding: [0xfb,0x55]
cmov.ne r2, r6
# CHECK: cmov.ne r2, r6{{.*}}encoding: [0xfb,0x56]
cmov.ne r2, r7
# CHECK: cmov.ne r2, r7{{.*}}encoding: [0xfb,0x57]
cmov.ne r3, r0
# CHECK: cmov.ne r3, r0{{.*}}encoding: [0xfb,0x58]
cmov.ne r3, r1
# CHECK: cmov.ne r3, r1{{.*}}encoding: [0xfb,0x59]
cmov.ne r3, r2
# CHECK: cmov.ne r3, r2{{.*}}encoding: [0xfb,0x5a]
cmov.ne r3, r3
# CHECK: cmov.ne r3, r3{{.*}}encoding: [0xfb,0x5b]
cmov.ne r3, r4
# CHECK: cmov.ne r3, r4{{.*}}encoding: [0xfb,0x5c]
cmov.ne r3, r5
# CHECK: cmov.ne r3, r5{{.*}}encoding: [0xfb,0x5d]
cmov.ne r3, r6
# CHECK: cmov.ne r3, r6{{.*}}encoding: [0xfb,0x5e]
cmov.ne r3, r7
# CHECK: cmov.ne r3, r7{{.*}}encoding: [0xfb,0x5f]
cmov.ne r4, r0
# CHECK: cmov.ne r4, r0{{.*}}encoding: [0xfb,0x60]
cmov.ne r4, r1
# CHECK: cmov.ne r4, r1{{.*}}encoding: [0xfb,0x61]
cmov.ne r4, r2
# CHECK: cmov.ne r4, r2{{.*}}encoding: [0xfb,0x62]
cmov.ne r4, r3
# CHECK: cmov.ne r4, r3{{.*}}encoding: [0xfb,0x63]
cmov.ne r4, r4
# CHECK: cmov.ne r4, r4{{.*}}encoding: [0xfb,0x64]
cmov.ne r4, r5
# CHECK: cmov.ne r4, r5{{.*}}encoding: [0xfb,0x65]
cmov.ne r4, r6
# CHECK: cmov.ne r4, r6{{.*}}encoding: [0xfb,0x66]
cmov.ne r4, r7
# CHECK: cmov.ne r4, r7{{.*}}encoding: [0xfb,0x67]
cmov.ne r5, r0
# CHECK: cmov.ne r5, r0{{.*}}encoding: [0xfb,0x68]
cmov.ne r5, r1
# CHECK: cmov.ne r5, r1{{.*}}encoding: [0xfb,0x69]
cmov.ne r5, r2
# CHECK: cmov.ne r5, r2{{.*}}encoding: [0xfb,0x6a]
cmov.ne r5, r3
# CHECK: cmov.ne r5, r3{{.*}}encoding: [0xfb,0x6b]
cmov.ne r5, r4
# CHECK: cmov.ne r5, r4{{.*}}encoding: [0xfb,0x6c]
cmov.ne r5, r5
# CHECK: cmov.ne r5, r5{{.*}}encoding: [0xfb,0x6d]
cmov.ne r5, r6
# CHECK: cmov.ne r5, r6{{.*}}encoding: [0xfb,0x6e]
cmov.ne r5, r7
# CHECK: cmov.ne r5, r7{{.*}}encoding: [0xfb,0x6f]
cmov.ne r6, r0
# CHECK: cmov.ne r6, r0{{.*}}encoding: [0xfb,0x70]
cmov.ne r6, r1
# CHECK: cmov.ne r6, r1{{.*}}encoding: [0xfb,0x71]
cmov.ne r6, r2
# CHECK: cmov.ne r6, r2{{.*}}encoding: [0xfb,0x72]
cmov.ne r6, r3
# CHECK: cmov.ne r6, r3{{.*}}encoding: [0xfb,0x73]
cmov.ne r6, r4
# CHECK: cmov.ne r6, r4{{.*}}encoding: [0xfb,0x74]
cmov.ne r6, r5
# CHECK: cmov.ne r6, r5{{.*}}encoding: [0xfb,0x75]
cmov.ne r6, r6
# CHECK: cmov.ne r6, r6{{.*}}encoding: [0xfb,0x76]
cmov.ne r6, r7
# CHECK: cmov.ne r6, r7{{.*}}encoding: [0xfb,0x77]
cmov.ne r7, r0
# CHECK: cmov.ne r7, r0{{.*}}encoding: [0xfb,0x78]
cmov.ne r7, r1
# CHECK: cmov.ne r7, r1{{.*}}encoding: [0xfb,0x79]
cmov.ne r7, r2
# CHECK: cmov.ne r7, r2{{.*}}encoding: [0xfb,0x7a]
cmov.ne r7, r3
# CHECK: cmov.ne r7, r3{{.*}}encoding: [0xfb,0x7b]
cmov.ne r7, r4
# CHECK: cmov.ne r7, r4{{.*}}encoding: [0xfb,0x7c]
cmov.ne r7, r5
# CHECK: cmov.ne r7, r5{{.*}}encoding: [0xfb,0x7d]
cmov.ne r7, r6
# CHECK: cmov.ne r7, r6{{.*}}encoding: [0xfb,0x7e]
cmov.ne r7, r7
# CHECK: cmov.ne r7, r7{{.*}}encoding: [0xfb,0x7f]
cmov.ult r0, r0
# CHECK: cmov.ult r0, r0{{.*}}encoding: [0xfc,0x00]
cmov.ult r0, r1
# CHECK: cmov.ult r0, r1{{.*}}encoding: [0xfc,0x01]
cmov.ult r0, r2
# CHECK: cmov.ult r0, r2{{.*}}encoding: [0xfc,0x02]
cmov.ult r0, r3
# CHECK: cmov.ult r0, r3{{.*}}encoding: [0xfc,0x03]
cmov.ult r0, r4
# CHECK: cmov.ult r0, r4{{.*}}encoding: [0xfc,0x04]
cmov.ult r0, r5
# CHECK: cmov.ult r0, r5{{.*}}encoding: [0xfc,0x05]
cmov.ult r0, r6
# CHECK: cmov.ult r0, r6{{.*}}encoding: [0xfc,0x06]
cmov.ult r0, r7
# CHECK: cmov.ult r0, r7{{.*}}encoding: [0xfc,0x07]
cmov.ult r1, r0
# CHECK: cmov.ult r1, r0{{.*}}encoding: [0xfc,0x08]
cmov.ult r1, r1
# CHECK: cmov.ult r1, r1{{.*}}encoding: [0xfc,0x09]
cmov.ult r1, r2
# CHECK: cmov.ult r1, r2{{.*}}encoding: [0xfc,0x0a]
cmov.ult r1, r3
# CHECK: cmov.ult r1, r3{{.*}}encoding: [0xfc,0x0b]
cmov.ult r1, r4
# CHECK: cmov.ult r1, r4{{.*}}encoding: [0xfc,0x0c]
cmov.ult r1, r5
# CHECK: cmov.ult r1, r5{{.*}}encoding: [0xfc,0x0d]
cmov.ult r1, r6
# CHECK: cmov.ult r1, r6{{.*}}encoding: [0xfc,0x0e]
cmov.ult r1, r7
# CHECK: cmov.ult r1, r7{{.*}}encoding: [0xfc,0x0f]
cmov.ult r2, r0
# CHECK: cmov.ult r2, r0{{.*}}encoding: [0xfc,0x10]
cmov.ult r2, r1
# CHECK: cmov.ult r2, r1{{.*}}encoding: [0xfc,0x11]
cmov.ult r2, r2
# CHECK: cmov.ult r2, r2{{.*}}encoding: [0xfc,0x12]
cmov.ult r2, r3
# CHECK: cmov.ult r2, r3{{.*}}encoding: [0xfc,0x13]
cmov.ult r2, r4
# CHECK: cmov.ult r2, r4{{.*}}encoding: [0xfc,0x14]
cmov.ult r2, r5
# CHECK: cmov.ult r2, r5{{.*}}encoding: [0xfc,0x15]
cmov.ult r2, r6
# CHECK: cmov.ult r2, r6{{.*}}encoding: [0xfc,0x16]
cmov.ult r2, r7
# CHECK: cmov.ult r2, r7{{.*}}encoding: [0xfc,0x17]
cmov.ult r3, r0
# CHECK: cmov.ult r3, r0{{.*}}encoding: [0xfc,0x18]
cmov.ult r3, r1
# CHECK: cmov.ult r3, r1{{.*}}encoding: [0xfc,0x19]
cmov.ult r3, r2
# CHECK: cmov.ult r3, r2{{.*}}encoding: [0xfc,0x1a]
cmov.ult r3, r3
# CHECK: cmov.ult r3, r3{{.*}}encoding: [0xfc,0x1b]
cmov.ult r3, r4
# CHECK: cmov.ult r3, r4{{.*}}encoding: [0xfc,0x1c]
cmov.ult r3, r5
# CHECK: cmov.ult r3, r5{{.*}}encoding: [0xfc,0x1d]
cmov.ult r3, r6
# CHECK: cmov.ult r3, r6{{.*}}encoding: [0xfc,0x1e]
cmov.ult r3, r7
# CHECK: cmov.ult r3, r7{{.*}}encoding: [0xfc,0x1f]
cmov.ult r4, r0
# CHECK: cmov.ult r4, r0{{.*}}encoding: [0xfc,0x20]
cmov.ult r4, r1
# CHECK: cmov.ult r4, r1{{.*}}encoding: [0xfc,0x21]
cmov.ult r4, r2
# CHECK: cmov.ult r4, r2{{.*}}encoding: [0xfc,0x22]
cmov.ult r4, r3
# CHECK: cmov.ult r4, r3{{.*}}encoding: [0xfc,0x23]
cmov.ult r4, r4
# CHECK: cmov.ult r4, r4{{.*}}encoding: [0xfc,0x24]
cmov.ult r4, r5
# CHECK: cmov.ult r4, r5{{.*}}encoding: [0xfc,0x25]
cmov.ult r4, r6
# CHECK: cmov.ult r4, r6{{.*}}encoding: [0xfc,0x26]
cmov.ult r4, r7
# CHECK: cmov.ult r4, r7{{.*}}encoding: [0xfc,0x27]
cmov.ult r5, r0
# CHECK: cmov.ult r5, r0{{.*}}encoding: [0xfc,0x28]
cmov.ult r5, r1
# CHECK: cmov.ult r5, r1{{.*}}encoding: [0xfc,0x29]
cmov.ult r5, r2
# CHECK: cmov.ult r5, r2{{.*}}encoding: [0xfc,0x2a]
cmov.ult r5, r3
# CHECK: cmov.ult r5, r3{{.*}}encoding: [0xfc,0x2b]
cmov.ult r5, r4
# CHECK: cmov.ult r5, r4{{.*}}encoding: [0xfc,0x2c]
cmov.ult r5, r5
# CHECK: cmov.ult r5, r5{{.*}}encoding: [0xfc,0x2d]
cmov.ult r5, r6
# CHECK: cmov.ult r5, r6{{.*}}encoding: [0xfc,0x2e]
cmov.ult r5, r7
# CHECK: cmov.ult r5, r7{{.*}}encoding: [0xfc,0x2f]
cmov.ult r6, r0
# CHECK: cmov.ult r6, r0{{.*}}encoding: [0xfc,0x30]
cmov.ult r6, r1
# CHECK: cmov.ult r6, r1{{.*}}encoding: [0xfc,0x31]
cmov.ult r6, r2
# CHECK: cmov.ult r6, r2{{.*}}encoding: [0xfc,0x32]
cmov.ult r6, r3
# CHECK: cmov.ult r6, r3{{.*}}encoding: [0xfc,0x33]
cmov.ult r6, r4
# CHECK: cmov.ult r6, r4{{.*}}encoding: [0xfc,0x34]
cmov.ult r6, r5
# CHECK: cmov.ult r6, r5{{.*}}encoding: [0xfc,0x35]
cmov.ult r6, r6
# CHECK: cmov.ult r6, r6{{.*}}encoding: [0xfc,0x36]
cmov.ult r6, r7
# CHECK: cmov.ult r6, r7{{.*}}encoding: [0xfc,0x37]
cmov.ult r7, r0
# CHECK: cmov.ult r7, r0{{.*}}encoding: [0xfc,0x38]
cmov.ult r7, r1
# CHECK: cmov.ult r7, r1{{.*}}encoding: [0xfc,0x39]
cmov.ult r7, r2
# CHECK: cmov.ult r7, r2{{.*}}encoding: [0xfc,0x3a]
cmov.ult r7, r3
# CHECK: cmov.ult r7, r3{{.*}}encoding: [0xfc,0x3b]
cmov.ult r7, r4
# CHECK: cmov.ult r7, r4{{.*}}encoding: [0xfc,0x3c]
cmov.ult r7, r5
# CHECK: cmov.ult r7, r5{{.*}}encoding: [0xfc,0x3d]
cmov.ult r7, r6
# CHECK: cmov.ult r7, r6{{.*}}encoding: [0xfc,0x3e]
cmov.ult r7, r7
# CHECK: cmov.ult r7, r7{{.*}}encoding: [0xfc,0x3f]
cmov.uge r0, r0
# CHECK: cmov.uge r0, r0{{.*}}encoding: [0xfc,0x40]
cmov.uge r0, r1
# CHECK: cmov.uge r0, r1{{.*}}encoding: [0xfc,0x41]
cmov.uge r0, r2
# CHECK: cmov.uge r0, r2{{.*}}encoding: [0xfc,0x42]
cmov.uge r0, r3
# CHECK: cmov.uge r0, r3{{.*}}encoding: [0xfc,0x43]
cmov.uge r0, r4
# CHECK: cmov.uge r0, r4{{.*}}encoding: [0xfc,0x44]
cmov.uge r0, r5
# CHECK: cmov.uge r0, r5{{.*}}encoding: [0xfc,0x45]
cmov.uge r0, r6
# CHECK: cmov.uge r0, r6{{.*}}encoding: [0xfc,0x46]
cmov.uge r0, r7
# CHECK: cmov.uge r0, r7{{.*}}encoding: [0xfc,0x47]
cmov.uge r1, r0
# CHECK: cmov.uge r1, r0{{.*}}encoding: [0xfc,0x48]
cmov.uge r1, r1
# CHECK: cmov.uge r1, r1{{.*}}encoding: [0xfc,0x49]
cmov.uge r1, r2
# CHECK: cmov.uge r1, r2{{.*}}encoding: [0xfc,0x4a]
cmov.uge r1, r3
# CHECK: cmov.uge r1, r3{{.*}}encoding: [0xfc,0x4b]
cmov.uge r1, r4
# CHECK: cmov.uge r1, r4{{.*}}encoding: [0xfc,0x4c]
cmov.uge r1, r5
# CHECK: cmov.uge r1, r5{{.*}}encoding: [0xfc,0x4d]
cmov.uge r1, r6
# CHECK: cmov.uge r1, r6{{.*}}encoding: [0xfc,0x4e]
cmov.uge r1, r7
# CHECK: cmov.uge r1, r7{{.*}}encoding: [0xfc,0x4f]
cmov.uge r2, r0
# CHECK: cmov.uge r2, r0{{.*}}encoding: [0xfc,0x50]
cmov.uge r2, r1
# CHECK: cmov.uge r2, r1{{.*}}encoding: [0xfc,0x51]
cmov.uge r2, r2
# CHECK: cmov.uge r2, r2{{.*}}encoding: [0xfc,0x52]
cmov.uge r2, r3
# CHECK: cmov.uge r2, r3{{.*}}encoding: [0xfc,0x53]
cmov.uge r2, r4
# CHECK: cmov.uge r2, r4{{.*}}encoding: [0xfc,0x54]
cmov.uge r2, r5
# CHECK: cmov.uge r2, r5{{.*}}encoding: [0xfc,0x55]
cmov.uge r2, r6
# CHECK: cmov.uge r2, r6{{.*}}encoding: [0xfc,0x56]
cmov.uge r2, r7
# CHECK: cmov.uge r2, r7{{.*}}encoding: [0xfc,0x57]
cmov.uge r3, r0
# CHECK: cmov.uge r3, r0{{.*}}encoding: [0xfc,0x58]
cmov.uge r3, r1
# CHECK: cmov.uge r3, r1{{.*}}encoding: [0xfc,0x59]
cmov.uge r3, r2
# CHECK: cmov.uge r3, r2{{.*}}encoding: [0xfc,0x5a]
cmov.uge r3, r3
# CHECK: cmov.uge r3, r3{{.*}}encoding: [0xfc,0x5b]
cmov.uge r3, r4
# CHECK: cmov.uge r3, r4{{.*}}encoding: [0xfc,0x5c]
cmov.uge r3, r5
# CHECK: cmov.uge r3, r5{{.*}}encoding: [0xfc,0x5d]
cmov.uge r3, r6
# CHECK: cmov.uge r3, r6{{.*}}encoding: [0xfc,0x5e]
cmov.uge r3, r7
# CHECK: cmov.uge r3, r7{{.*}}encoding: [0xfc,0x5f]
cmov.uge r4, r0
# CHECK: cmov.uge r4, r0{{.*}}encoding: [0xfc,0x60]
cmov.uge r4, r1
# CHECK: cmov.uge r4, r1{{.*}}encoding: [0xfc,0x61]
cmov.uge r4, r2
# CHECK: cmov.uge r4, r2{{.*}}encoding: [0xfc,0x62]
cmov.uge r4, r3
# CHECK: cmov.uge r4, r3{{.*}}encoding: [0xfc,0x63]
cmov.uge r4, r4
# CHECK: cmov.uge r4, r4{{.*}}encoding: [0xfc,0x64]
cmov.uge r4, r5
# CHECK: cmov.uge r4, r5{{.*}}encoding: [0xfc,0x65]
cmov.uge r4, r6
# CHECK: cmov.uge r4, r6{{.*}}encoding: [0xfc,0x66]
cmov.uge r4, r7
# CHECK: cmov.uge r4, r7{{.*}}encoding: [0xfc,0x67]
cmov.uge r5, r0
# CHECK: cmov.uge r5, r0{{.*}}encoding: [0xfc,0x68]
cmov.uge r5, r1
# CHECK: cmov.uge r5, r1{{.*}}encoding: [0xfc,0x69]
cmov.uge r5, r2
# CHECK: cmov.uge r5, r2{{.*}}encoding: [0xfc,0x6a]
cmov.uge r5, r3
# CHECK: cmov.uge r5, r3{{.*}}encoding: [0xfc,0x6b]
cmov.uge r5, r4
# CHECK: cmov.uge r5, r4{{.*}}encoding: [0xfc,0x6c]
cmov.uge r5, r5
# CHECK: cmov.uge r5, r5{{.*}}encoding: [0xfc,0x6d]
cmov.uge r5, r6
# CHECK: cmov.uge r5, r6{{.*}}encoding: [0xfc,0x6e]
cmov.uge r5, r7
# CHECK: cmov.uge r5, r7{{.*}}encoding: [0xfc,0x6f]
cmov.uge r6, r0
# CHECK: cmov.uge r6, r0{{.*}}encoding: [0xfc,0x70]
cmov.uge r6, r1
# CHECK: cmov.uge r6, r1{{.*}}encoding: [0xfc,0x71]
cmov.uge r6, r2
# CHECK: cmov.uge r6, r2{{.*}}encoding: [0xfc,0x72]
cmov.uge r6, r3
# CHECK: cmov.uge r6, r3{{.*}}encoding: [0xfc,0x73]
cmov.uge r6, r4
# CHECK: cmov.uge r6, r4{{.*}}encoding: [0xfc,0x74]
cmov.uge r6, r5
# CHECK: cmov.uge r6, r5{{.*}}encoding: [0xfc,0x75]
cmov.uge r6, r6
# CHECK: cmov.uge r6, r6{{.*}}encoding: [0xfc,0x76]
cmov.uge r6, r7
# CHECK: cmov.uge r6, r7{{.*}}encoding: [0xfc,0x77]
cmov.uge r7, r0
# CHECK: cmov.uge r7, r0{{.*}}encoding: [0xfc,0x78]
cmov.uge r7, r1
# CHECK: cmov.uge r7, r1{{.*}}encoding: [0xfc,0x79]
cmov.uge r7, r2
# CHECK: cmov.uge r7, r2{{.*}}encoding: [0xfc,0x7a]
cmov.uge r7, r3
# CHECK: cmov.uge r7, r3{{.*}}encoding: [0xfc,0x7b]
cmov.uge r7, r4
# CHECK: cmov.uge r7, r4{{.*}}encoding: [0xfc,0x7c]
cmov.uge r7, r5
# CHECK: cmov.uge r7, r5{{.*}}encoding: [0xfc,0x7d]
cmov.uge r7, r6
# CHECK: cmov.uge r7, r6{{.*}}encoding: [0xfc,0x7e]
cmov.uge r7, r7
# CHECK: cmov.uge r7, r7{{.*}}encoding: [0xfc,0x7f]
cmov.slt r0, r0
# CHECK: cmov.slt r0, r0{{.*}}encoding: [0xfd,0x00]
cmov.slt r0, r1
# CHECK: cmov.slt r0, r1{{.*}}encoding: [0xfd,0x01]
cmov.slt r0, r2
# CHECK: cmov.slt r0, r2{{.*}}encoding: [0xfd,0x02]
cmov.slt r0, r3
# CHECK: cmov.slt r0, r3{{.*}}encoding: [0xfd,0x03]
cmov.slt r0, r4
# CHECK: cmov.slt r0, r4{{.*}}encoding: [0xfd,0x04]
cmov.slt r0, r5
# CHECK: cmov.slt r0, r5{{.*}}encoding: [0xfd,0x05]
cmov.slt r0, r6
# CHECK: cmov.slt r0, r6{{.*}}encoding: [0xfd,0x06]
cmov.slt r0, r7
# CHECK: cmov.slt r0, r7{{.*}}encoding: [0xfd,0x07]
cmov.slt r1, r0
# CHECK: cmov.slt r1, r0{{.*}}encoding: [0xfd,0x08]
cmov.slt r1, r1
# CHECK: cmov.slt r1, r1{{.*}}encoding: [0xfd,0x09]
cmov.slt r1, r2
# CHECK: cmov.slt r1, r2{{.*}}encoding: [0xfd,0x0a]
cmov.slt r1, r3
# CHECK: cmov.slt r1, r3{{.*}}encoding: [0xfd,0x0b]
cmov.slt r1, r4
# CHECK: cmov.slt r1, r4{{.*}}encoding: [0xfd,0x0c]
cmov.slt r1, r5
# CHECK: cmov.slt r1, r5{{.*}}encoding: [0xfd,0x0d]
cmov.slt r1, r6
# CHECK: cmov.slt r1, r6{{.*}}encoding: [0xfd,0x0e]
cmov.slt r1, r7
# CHECK: cmov.slt r1, r7{{.*}}encoding: [0xfd,0x0f]
cmov.slt r2, r0
# CHECK: cmov.slt r2, r0{{.*}}encoding: [0xfd,0x10]
cmov.slt r2, r1
# CHECK: cmov.slt r2, r1{{.*}}encoding: [0xfd,0x11]
cmov.slt r2, r2
# CHECK: cmov.slt r2, r2{{.*}}encoding: [0xfd,0x12]
cmov.slt r2, r3
# CHECK: cmov.slt r2, r3{{.*}}encoding: [0xfd,0x13]
cmov.slt r2, r4
# CHECK: cmov.slt r2, r4{{.*}}encoding: [0xfd,0x14]
cmov.slt r2, r5
# CHECK: cmov.slt r2, r5{{.*}}encoding: [0xfd,0x15]
cmov.slt r2, r6
# CHECK: cmov.slt r2, r6{{.*}}encoding: [0xfd,0x16]
cmov.slt r2, r7
# CHECK: cmov.slt r2, r7{{.*}}encoding: [0xfd,0x17]
cmov.slt r3, r0
# CHECK: cmov.slt r3, r0{{.*}}encoding: [0xfd,0x18]
cmov.slt r3, r1
# CHECK: cmov.slt r3, r1{{.*}}encoding: [0xfd,0x19]
cmov.slt r3, r2
# CHECK: cmov.slt r3, r2{{.*}}encoding: [0xfd,0x1a]
cmov.slt r3, r3
# CHECK: cmov.slt r3, r3{{.*}}encoding: [0xfd,0x1b]
cmov.slt r3, r4
# CHECK: cmov.slt r3, r4{{.*}}encoding: [0xfd,0x1c]
cmov.slt r3, r5
# CHECK: cmov.slt r3, r5{{.*}}encoding: [0xfd,0x1d]
cmov.slt r3, r6
# CHECK: cmov.slt r3, r6{{.*}}encoding: [0xfd,0x1e]
cmov.slt r3, r7
# CHECK: cmov.slt r3, r7{{.*}}encoding: [0xfd,0x1f]
cmov.slt r4, r0
# CHECK: cmov.slt r4, r0{{.*}}encoding: [0xfd,0x20]
cmov.slt r4, r1
# CHECK: cmov.slt r4, r1{{.*}}encoding: [0xfd,0x21]
cmov.slt r4, r2
# CHECK: cmov.slt r4, r2{{.*}}encoding: [0xfd,0x22]
cmov.slt r4, r3
# CHECK: cmov.slt r4, r3{{.*}}encoding: [0xfd,0x23]
cmov.slt r4, r4
# CHECK: cmov.slt r4, r4{{.*}}encoding: [0xfd,0x24]
cmov.slt r4, r5
# CHECK: cmov.slt r4, r5{{.*}}encoding: [0xfd,0x25]
cmov.slt r4, r6
# CHECK: cmov.slt r4, r6{{.*}}encoding: [0xfd,0x26]
cmov.slt r4, r7
# CHECK: cmov.slt r4, r7{{.*}}encoding: [0xfd,0x27]
cmov.slt r5, r0
# CHECK: cmov.slt r5, r0{{.*}}encoding: [0xfd,0x28]
cmov.slt r5, r1
# CHECK: cmov.slt r5, r1{{.*}}encoding: [0xfd,0x29]
cmov.slt r5, r2
# CHECK: cmov.slt r5, r2{{.*}}encoding: [0xfd,0x2a]
cmov.slt r5, r3
# CHECK: cmov.slt r5, r3{{.*}}encoding: [0xfd,0x2b]
cmov.slt r5, r4
# CHECK: cmov.slt r5, r4{{.*}}encoding: [0xfd,0x2c]
cmov.slt r5, r5
# CHECK: cmov.slt r5, r5{{.*}}encoding: [0xfd,0x2d]
cmov.slt r5, r6
# CHECK: cmov.slt r5, r6{{.*}}encoding: [0xfd,0x2e]
cmov.slt r5, r7
# CHECK: cmov.slt r5, r7{{.*}}encoding: [0xfd,0x2f]
cmov.slt r6, r0
# CHECK: cmov.slt r6, r0{{.*}}encoding: [0xfd,0x30]
cmov.slt r6, r1
# CHECK: cmov.slt r6, r1{{.*}}encoding: [0xfd,0x31]
cmov.slt r6, r2
# CHECK: cmov.slt r6, r2{{.*}}encoding: [0xfd,0x32]
cmov.slt r6, r3
# CHECK: cmov.slt r6, r3{{.*}}encoding: [0xfd,0x33]
cmov.slt r6, r4
# CHECK: cmov.slt r6, r4{{.*}}encoding: [0xfd,0x34]
cmov.slt r6, r5
# CHECK: cmov.slt r6, r5{{.*}}encoding: [0xfd,0x35]
cmov.slt r6, r6
# CHECK: cmov.slt r6, r6{{.*}}encoding: [0xfd,0x36]
cmov.slt r6, r7
# CHECK: cmov.slt r6, r7{{.*}}encoding: [0xfd,0x37]
cmov.slt r7, r0
# CHECK: cmov.slt r7, r0{{.*}}encoding: [0xfd,0x38]
cmov.slt r7, r1
# CHECK: cmov.slt r7, r1{{.*}}encoding: [0xfd,0x39]
cmov.slt r7, r2
# CHECK: cmov.slt r7, r2{{.*}}encoding: [0xfd,0x3a]
cmov.slt r7, r3
# CHECK: cmov.slt r7, r3{{.*}}encoding: [0xfd,0x3b]
cmov.slt r7, r4
# CHECK: cmov.slt r7, r4{{.*}}encoding: [0xfd,0x3c]
cmov.slt r7, r5
# CHECK: cmov.slt r7, r5{{.*}}encoding: [0xfd,0x3d]
cmov.slt r7, r6
# CHECK: cmov.slt r7, r6{{.*}}encoding: [0xfd,0x3e]
cmov.slt r7, r7
# CHECK: cmov.slt r7, r7{{.*}}encoding: [0xfd,0x3f]
cmov.sge r0, r0
# CHECK: cmov.sge r0, r0{{.*}}encoding: [0xfd,0x40]
cmov.sge r0, r1
# CHECK: cmov.sge r0, r1{{.*}}encoding: [0xfd,0x41]
cmov.sge r0, r2
# CHECK: cmov.sge r0, r2{{.*}}encoding: [0xfd,0x42]
cmov.sge r0, r3
# CHECK: cmov.sge r0, r3{{.*}}encoding: [0xfd,0x43]
cmov.sge r0, r4
# CHECK: cmov.sge r0, r4{{.*}}encoding: [0xfd,0x44]
cmov.sge r0, r5
# CHECK: cmov.sge r0, r5{{.*}}encoding: [0xfd,0x45]
cmov.sge r0, r6
# CHECK: cmov.sge r0, r6{{.*}}encoding: [0xfd,0x46]
cmov.sge r0, r7
# CHECK: cmov.sge r0, r7{{.*}}encoding: [0xfd,0x47]
cmov.sge r1, r0
# CHECK: cmov.sge r1, r0{{.*}}encoding: [0xfd,0x48]
cmov.sge r1, r1
# CHECK: cmov.sge r1, r1{{.*}}encoding: [0xfd,0x49]
cmov.sge r1, r2
# CHECK: cmov.sge r1, r2{{.*}}encoding: [0xfd,0x4a]
cmov.sge r1, r3
# CHECK: cmov.sge r1, r3{{.*}}encoding: [0xfd,0x4b]
cmov.sge r1, r4
# CHECK: cmov.sge r1, r4{{.*}}encoding: [0xfd,0x4c]
cmov.sge r1, r5
# CHECK: cmov.sge r1, r5{{.*}}encoding: [0xfd,0x4d]
cmov.sge r1, r6
# CHECK: cmov.sge r1, r6{{.*}}encoding: [0xfd,0x4e]
cmov.sge r1, r7
# CHECK: cmov.sge r1, r7{{.*}}encoding: [0xfd,0x4f]
cmov.sge r2, r0
# CHECK: cmov.sge r2, r0{{.*}}encoding: [0xfd,0x50]
cmov.sge r2, r1
# CHECK: cmov.sge r2, r1{{.*}}encoding: [0xfd,0x51]
cmov.sge r2, r2
# CHECK: cmov.sge r2, r2{{.*}}encoding: [0xfd,0x52]
cmov.sge r2, r3
# CHECK: cmov.sge r2, r3{{.*}}encoding: [0xfd,0x53]
cmov.sge r2, r4
# CHECK: cmov.sge r2, r4{{.*}}encoding: [0xfd,0x54]
cmov.sge r2, r5
# CHECK: cmov.sge r2, r5{{.*}}encoding: [0xfd,0x55]
cmov.sge r2, r6
# CHECK: cmov.sge r2, r6{{.*}}encoding: [0xfd,0x56]
cmov.sge r2, r7
# CHECK: cmov.sge r2, r7{{.*}}encoding: [0xfd,0x57]
cmov.sge r3, r0
# CHECK: cmov.sge r3, r0{{.*}}encoding: [0xfd,0x58]
cmov.sge r3, r1
# CHECK: cmov.sge r3, r1{{.*}}encoding: [0xfd,0x59]
cmov.sge r3, r2
# CHECK: cmov.sge r3, r2{{.*}}encoding: [0xfd,0x5a]
cmov.sge r3, r3
# CHECK: cmov.sge r3, r3{{.*}}encoding: [0xfd,0x5b]
cmov.sge r3, r4
# CHECK: cmov.sge r3, r4{{.*}}encoding: [0xfd,0x5c]
cmov.sge r3, r5
# CHECK: cmov.sge r3, r5{{.*}}encoding: [0xfd,0x5d]
cmov.sge r3, r6
# CHECK: cmov.sge r3, r6{{.*}}encoding: [0xfd,0x5e]
cmov.sge r3, r7
# CHECK: cmov.sge r3, r7{{.*}}encoding: [0xfd,0x5f]
cmov.sge r4, r0
# CHECK: cmov.sge r4, r0{{.*}}encoding: [0xfd,0x60]
cmov.sge r4, r1
# CHECK: cmov.sge r4, r1{{.*}}encoding: [0xfd,0x61]
cmov.sge r4, r2
# CHECK: cmov.sge r4, r2{{.*}}encoding: [0xfd,0x62]
cmov.sge r4, r3
# CHECK: cmov.sge r4, r3{{.*}}encoding: [0xfd,0x63]
cmov.sge r4, r4
# CHECK: cmov.sge r4, r4{{.*}}encoding: [0xfd,0x64]
cmov.sge r4, r5
# CHECK: cmov.sge r4, r5{{.*}}encoding: [0xfd,0x65]
cmov.sge r4, r6
# CHECK: cmov.sge r4, r6{{.*}}encoding: [0xfd,0x66]
cmov.sge r4, r7
# CHECK: cmov.sge r4, r7{{.*}}encoding: [0xfd,0x67]
cmov.sge r5, r0
# CHECK: cmov.sge r5, r0{{.*}}encoding: [0xfd,0x68]
cmov.sge r5, r1
# CHECK: cmov.sge r5, r1{{.*}}encoding: [0xfd,0x69]
cmov.sge r5, r2
# CHECK: cmov.sge r5, r2{{.*}}encoding: [0xfd,0x6a]
cmov.sge r5, r3
# CHECK: cmov.sge r5, r3{{.*}}encoding: [0xfd,0x6b]
cmov.sge r5, r4
# CHECK: cmov.sge r5, r4{{.*}}encoding: [0xfd,0x6c]
cmov.sge r5, r5
# CHECK: cmov.sge r5, r5{{.*}}encoding: [0xfd,0x6d]
cmov.sge r5, r6
# CHECK: cmov.sge r5, r6{{.*}}encoding: [0xfd,0x6e]
cmov.sge r5, r7
# CHECK: cmov.sge r5, r7{{.*}}encoding: [0xfd,0x6f]
cmov.sge r6, r0
# CHECK: cmov.sge r6, r0{{.*}}encoding: [0xfd,0x70]
cmov.sge r6, r1
# CHECK: cmov.sge r6, r1{{.*}}encoding: [0xfd,0x71]
cmov.sge r6, r2
# CHECK: cmov.sge r6, r2{{.*}}encoding: [0xfd,0x72]
cmov.sge r6, r3
# CHECK: cmov.sge r6, r3{{.*}}encoding: [0xfd,0x73]
cmov.sge r6, r4
# CHECK: cmov.sge r6, r4{{.*}}encoding: [0xfd,0x74]
cmov.sge r6, r5
# CHECK: cmov.sge r6, r5{{.*}}encoding: [0xfd,0x75]
cmov.sge r6, r6
# CHECK: cmov.sge r6, r6{{.*}}encoding: [0xfd,0x76]
cmov.sge r6, r7
# CHECK: cmov.sge r6, r7{{.*}}encoding: [0xfd,0x77]
cmov.sge r7, r0
# CHECK: cmov.sge r7, r0{{.*}}encoding: [0xfd,0x78]
cmov.sge r7, r1
# CHECK: cmov.sge r7, r1{{.*}}encoding: [0xfd,0x79]
cmov.sge r7, r2
# CHECK: cmov.sge r7, r2{{.*}}encoding: [0xfd,0x7a]
cmov.sge r7, r3
# CHECK: cmov.sge r7, r3{{.*}}encoding: [0xfd,0x7b]
cmov.sge r7, r4
# CHECK: cmov.sge r7, r4{{.*}}encoding: [0xfd,0x7c]
cmov.sge r7, r5
# CHECK: cmov.sge r7, r5{{.*}}encoding: [0xfd,0x7d]
cmov.sge r7, r6
# CHECK: cmov.sge r7, r6{{.*}}encoding: [0xfd,0x7e]
cmov.sge r7, r7
# CHECK: cmov.sge r7, r7{{.*}}encoding: [0xfd,0x7f]
