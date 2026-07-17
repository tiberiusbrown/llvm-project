# RUN: llvm-mc -triple=avm --show-encoding %s | FileCheck %s

fadd q0, q0
fadd q0, q1
fadd q0, q2
fadd q0, q3
fadd q1, q0
fadd q1, q1
fadd q1, q2
fadd q1, q3
fadd q2, q0
fadd q2, q1
fadd q2, q2
fadd q2, q3
fadd q3, q0
fadd q3, q1
fadd q3, q2
fadd q3, q3
fsub q0, q0
fsub q0, q1
fsub q0, q2
fsub q0, q3
fsub q1, q0
fsub q1, q1
fsub q1, q2
fsub q1, q3
fsub q2, q0
fsub q2, q1
fsub q2, q2
fsub q2, q3
fsub q3, q0
fsub q3, q1
fsub q3, q2
fsub q3, q3
fmul q0, q0
fmul q0, q1
fmul q0, q2
fmul q0, q3
fmul q1, q0
fmul q1, q1
fmul q1, q2
fmul q1, q3
fmul q2, q0
fmul q2, q1
fmul q2, q2
fmul q2, q3
fmul q3, q0
fmul q3, q1
fmul q3, q2
fmul q3, q3
fdiv q0, q0
fdiv q0, q1
fdiv q0, q2
fdiv q0, q3
fdiv q1, q0
fdiv q1, q1
fdiv q1, q2
fdiv q1, q3
fdiv q2, q0
fdiv q2, q1
fdiv q2, q2
fdiv q2, q3
fdiv q3, q0
fdiv q3, q1
fdiv q3, q2
fdiv q3, q3
fmin q0, q0
fmin q0, q1
fmin q0, q2
fmin q0, q3
fmin q1, q0
fmin q1, q1
fmin q1, q2
fmin q1, q3
fmin q2, q0
fmin q2, q1
fmin q2, q2
fmin q2, q3
fmin q3, q0
fmin q3, q1
fmin q3, q2
fmin q3, q3
fmax q0, q0
fmax q0, q1
fmax q0, q2
fmax q0, q3
fmax q1, q0
fmax q1, q1
fmax q1, q2
fmax q1, q3
fmax q2, q0
fmax q2, q1
fmax q2, q2
fmax q2, q3
fmax q3, q0
fmax q3, q1
fmax q3, q2
fmax q3, q3
fneg q0
fneg q1
fneg q2
fneg q3
fabs q0
fabs q1
fabs q2
fabs q3
fsqrt q0
fsqrt q1
fsqrt q2
fsqrt q3
ftrunc q0
ftrunc q1
ftrunc q2
ftrunc q3
ffloor q0
ffloor q1
ffloor q2
ffloor q3
fceil q0
fceil q1
fceil q2
fceil q3
fround q0
fround q1
fround q2
fround q3
s16tof q0, r0
s16tof q1, r0
s16tof q2, r0
s16tof q3, r0
s16tof q0, r1
s16tof q1, r1
s16tof q2, r1
s16tof q3, r1
s16tof q0, r2
s16tof q1, r2
s16tof q2, r2
s16tof q3, r2
s16tof q0, r3
s16tof q1, r3
s16tof q2, r3
s16tof q3, r3
s16tof q0, r4
s16tof q1, r4
s16tof q2, r4
s16tof q3, r4
s16tof q0, r5
s16tof q1, r5
s16tof q2, r5
s16tof q3, r5
s16tof q0, r6
s16tof q1, r6
s16tof q2, r6
s16tof q3, r6
s16tof q0, r7
s16tof q1, r7
s16tof q2, r7
s16tof q3, r7
u16tof q0, r0
u16tof q1, r0
u16tof q2, r0
u16tof q3, r0
u16tof q0, r1
u16tof q1, r1
u16tof q2, r1
u16tof q3, r1
u16tof q0, r2
u16tof q1, r2
u16tof q2, r2
u16tof q3, r2
u16tof q0, r3
u16tof q1, r3
u16tof q2, r3
u16tof q3, r3
u16tof q0, r4
u16tof q1, r4
u16tof q2, r4
u16tof q3, r4
u16tof q0, r5
u16tof q1, r5
u16tof q2, r5
u16tof q3, r5
u16tof q0, r6
u16tof q1, r6
u16tof q2, r6
u16tof q3, r6
u16tof q0, r7
u16tof q1, r7
u16tof q2, r7
u16tof q3, r7
ftos16 r0, q0
ftos16 r0, q1
ftos16 r0, q2
ftos16 r0, q3
ftos16 r1, q0
ftos16 r1, q1
ftos16 r1, q2
ftos16 r1, q3
ftos16 r2, q0
ftos16 r2, q1
ftos16 r2, q2
ftos16 r2, q3
ftos16 r3, q0
ftos16 r3, q1
ftos16 r3, q2
ftos16 r3, q3
ftos16 r4, q0
ftos16 r4, q1
ftos16 r4, q2
ftos16 r4, q3
ftos16 r5, q0
ftos16 r5, q1
ftos16 r5, q2
ftos16 r5, q3
ftos16 r6, q0
ftos16 r6, q1
ftos16 r6, q2
ftos16 r6, q3
ftos16 r7, q0
ftos16 r7, q1
ftos16 r7, q2
ftos16 r7, q3
ftou16 r0, q0
ftou16 r0, q1
ftou16 r0, q2
ftou16 r0, q3
ftou16 r1, q0
ftou16 r1, q1
ftou16 r1, q2
ftou16 r1, q3
ftou16 r2, q0
ftou16 r2, q1
ftou16 r2, q2
ftou16 r2, q3
ftou16 r3, q0
ftou16 r3, q1
ftou16 r3, q2
ftou16 r3, q3
ftou16 r4, q0
ftou16 r4, q1
ftou16 r4, q2
ftou16 r4, q3
ftou16 r5, q0
ftou16 r5, q1
ftou16 r5, q2
ftou16 r5, q3
ftou16 r6, q0
ftou16 r6, q1
ftou16 r6, q2
ftou16 r6, q3
ftou16 r7, q0
ftou16 r7, q1
ftou16 r7, q2
ftou16 r7, q3
fclass r0, q0
fclass r0, q1
fclass r0, q2
fclass r0, q3
fclass r1, q0
fclass r1, q1
fclass r1, q2
fclass r1, q3
fclass r2, q0
fclass r2, q1
fclass r2, q2
fclass r2, q3
fclass r3, q0
fclass r3, q1
fclass r3, q2
fclass r3, q3
fclass r4, q0
fclass r4, q1
fclass r4, q2
fclass r4, q3
fclass r5, q0
fclass r5, q1
fclass r5, q2
fclass r5, q3
fclass r6, q0
fclass r6, q1
fclass r6, q2
fclass r6, q3
fclass r7, q0
fclass r7, q1
fclass r7, q2
fclass r7, q3
s32tof q0, q0
s32tof q0, q1
s32tof q0, q2
s32tof q0, q3
s32tof q1, q0
s32tof q1, q1
s32tof q1, q2
s32tof q1, q3
s32tof q2, q0
s32tof q2, q1
s32tof q2, q2
s32tof q2, q3
s32tof q3, q0
s32tof q3, q1
s32tof q3, q2
s32tof q3, q3
u32tof q0, q0
u32tof q0, q1
u32tof q0, q2
u32tof q0, q3
u32tof q1, q0
u32tof q1, q1
u32tof q1, q2
u32tof q1, q3
u32tof q2, q0
u32tof q2, q1
u32tof q2, q2
u32tof q2, q3
u32tof q3, q0
u32tof q3, q1
u32tof q3, q2
u32tof q3, q3
ftos32 q0, q0
ftos32 q0, q1
ftos32 q0, q2
ftos32 q0, q3
ftos32 q1, q0
ftos32 q1, q1
ftos32 q1, q2
ftos32 q1, q3
ftos32 q2, q0
ftos32 q2, q1
ftos32 q2, q2
ftos32 q2, q3
ftos32 q3, q0
ftos32 q3, q1
ftos32 q3, q2
ftos32 q3, q3
ftou32 q0, q0
ftou32 q0, q1
ftou32 q0, q2
ftou32 q0, q3
ftou32 q1, q0
ftou32 q1, q1
ftou32 q1, q2
ftou32 q1, q3
ftou32 q2, q0
ftou32 q2, q1
ftou32 q2, q2
ftou32 q2, q3
ftou32 q3, q0
ftou32 q3, q1
ftou32 q3, q2
ftou32 q3, q3
fcmp r0, q0, q0
fcmp r0, q0, q1
fcmp r0, q0, q2
fcmp r0, q0, q3
fcmp r0, q1, q0
fcmp r0, q1, q1
fcmp r0, q1, q2
fcmp r0, q1, q3
fcmp r0, q2, q0
fcmp r0, q2, q1
fcmp r0, q2, q2
fcmp r0, q2, q3
fcmp r0, q3, q0
fcmp r0, q3, q1
fcmp r0, q3, q2
fcmp r0, q3, q3
fcmp r1, q0, q0
fcmp r1, q0, q1
fcmp r1, q0, q2
fcmp r1, q0, q3
fcmp r1, q1, q0
fcmp r1, q1, q1
fcmp r1, q1, q2
fcmp r1, q1, q3
fcmp r1, q2, q0
fcmp r1, q2, q1
fcmp r1, q2, q2
fcmp r1, q2, q3
fcmp r1, q3, q0
fcmp r1, q3, q1
fcmp r1, q3, q2
fcmp r1, q3, q3
fcmp r2, q0, q0
fcmp r2, q0, q1
fcmp r2, q0, q2
fcmp r2, q0, q3
fcmp r2, q1, q0
fcmp r2, q1, q1
fcmp r2, q1, q2
fcmp r2, q1, q3
fcmp r2, q2, q0
fcmp r2, q2, q1
fcmp r2, q2, q2
fcmp r2, q2, q3
fcmp r2, q3, q0
fcmp r2, q3, q1
fcmp r2, q3, q2
fcmp r2, q3, q3
fcmp r3, q0, q0
fcmp r3, q0, q1
fcmp r3, q0, q2
fcmp r3, q0, q3
fcmp r3, q1, q0
fcmp r3, q1, q1
fcmp r3, q1, q2
fcmp r3, q1, q3
fcmp r3, q2, q0
fcmp r3, q2, q1
fcmp r3, q2, q2
fcmp r3, q2, q3
fcmp r3, q3, q0
fcmp r3, q3, q1
fcmp r3, q3, q2
fcmp r3, q3, q3
fcmp r4, q0, q0
fcmp r4, q0, q1
fcmp r4, q0, q2
fcmp r4, q0, q3
fcmp r4, q1, q0
fcmp r4, q1, q1
fcmp r4, q1, q2
fcmp r4, q1, q3
fcmp r4, q2, q0
fcmp r4, q2, q1
fcmp r4, q2, q2
fcmp r4, q2, q3
fcmp r4, q3, q0
fcmp r4, q3, q1
fcmp r4, q3, q2
fcmp r4, q3, q3
fcmp r5, q0, q0
fcmp r5, q0, q1
fcmp r5, q0, q2
fcmp r5, q0, q3
fcmp r5, q1, q0
fcmp r5, q1, q1
fcmp r5, q1, q2
fcmp r5, q1, q3
fcmp r5, q2, q0
fcmp r5, q2, q1
fcmp r5, q2, q2
fcmp r5, q2, q3
fcmp r5, q3, q0
fcmp r5, q3, q1
fcmp r5, q3, q2
fcmp r5, q3, q3
fcmp r6, q0, q0
fcmp r6, q0, q1
fcmp r6, q0, q2
fcmp r6, q0, q3
fcmp r6, q1, q0
fcmp r6, q1, q1
fcmp r6, q1, q2
fcmp r6, q1, q3
fcmp r6, q2, q0
fcmp r6, q2, q1
fcmp r6, q2, q2
fcmp r6, q2, q3
fcmp r6, q3, q0
fcmp r6, q3, q1
fcmp r6, q3, q2
fcmp r6, q3, q3
fcmp r7, q0, q0
fcmp r7, q0, q1
fcmp r7, q0, q2
fcmp r7, q0, q3
fcmp r7, q1, q0
fcmp r7, q1, q1
fcmp r7, q1, q2
fcmp r7, q1, q3
fcmp r7, q2, q0
fcmp r7, q2, q1
fcmp r7, q2, q2
fcmp r7, q2, q3
fcmp r7, q3, q0
fcmp r7, q3, q1
fcmp r7, q3, q2
fcmp r7, q3, q3

# CHECK: fadd q0, q0{{.*}}encoding: [0xff,0x00]
# CHECK: fadd q0, q1{{.*}}encoding: [0xff,0x01]
# CHECK: fadd q0, q2{{.*}}encoding: [0xff,0x02]
# CHECK: fadd q0, q3{{.*}}encoding: [0xff,0x03]
# CHECK: fadd q1, q0{{.*}}encoding: [0xff,0x04]
# CHECK: fadd q1, q1{{.*}}encoding: [0xff,0x05]
# CHECK: fadd q1, q2{{.*}}encoding: [0xff,0x06]
# CHECK: fadd q1, q3{{.*}}encoding: [0xff,0x07]
# CHECK: fadd q2, q0{{.*}}encoding: [0xff,0x08]
# CHECK: fadd q2, q1{{.*}}encoding: [0xff,0x09]
# CHECK: fadd q2, q2{{.*}}encoding: [0xff,0x0a]
# CHECK: fadd q2, q3{{.*}}encoding: [0xff,0x0b]
# CHECK: fadd q3, q0{{.*}}encoding: [0xff,0x0c]
# CHECK: fadd q3, q1{{.*}}encoding: [0xff,0x0d]
# CHECK: fadd q3, q2{{.*}}encoding: [0xff,0x0e]
# CHECK: fadd q3, q3{{.*}}encoding: [0xff,0x0f]
# CHECK: fsub q0, q0{{.*}}encoding: [0xff,0x10]
# CHECK: fsub q0, q1{{.*}}encoding: [0xff,0x11]
# CHECK: fsub q0, q2{{.*}}encoding: [0xff,0x12]
# CHECK: fsub q0, q3{{.*}}encoding: [0xff,0x13]
# CHECK: fsub q1, q0{{.*}}encoding: [0xff,0x14]
# CHECK: fsub q1, q1{{.*}}encoding: [0xff,0x15]
# CHECK: fsub q1, q2{{.*}}encoding: [0xff,0x16]
# CHECK: fsub q1, q3{{.*}}encoding: [0xff,0x17]
# CHECK: fsub q2, q0{{.*}}encoding: [0xff,0x18]
# CHECK: fsub q2, q1{{.*}}encoding: [0xff,0x19]
# CHECK: fsub q2, q2{{.*}}encoding: [0xff,0x1a]
# CHECK: fsub q2, q3{{.*}}encoding: [0xff,0x1b]
# CHECK: fsub q3, q0{{.*}}encoding: [0xff,0x1c]
# CHECK: fsub q3, q1{{.*}}encoding: [0xff,0x1d]
# CHECK: fsub q3, q2{{.*}}encoding: [0xff,0x1e]
# CHECK: fsub q3, q3{{.*}}encoding: [0xff,0x1f]
# CHECK: fmul q0, q0{{.*}}encoding: [0xff,0x20]
# CHECK: fmul q0, q1{{.*}}encoding: [0xff,0x21]
# CHECK: fmul q0, q2{{.*}}encoding: [0xff,0x22]
# CHECK: fmul q0, q3{{.*}}encoding: [0xff,0x23]
# CHECK: fmul q1, q0{{.*}}encoding: [0xff,0x24]
# CHECK: fmul q1, q1{{.*}}encoding: [0xff,0x25]
# CHECK: fmul q1, q2{{.*}}encoding: [0xff,0x26]
# CHECK: fmul q1, q3{{.*}}encoding: [0xff,0x27]
# CHECK: fmul q2, q0{{.*}}encoding: [0xff,0x28]
# CHECK: fmul q2, q1{{.*}}encoding: [0xff,0x29]
# CHECK: fmul q2, q2{{.*}}encoding: [0xff,0x2a]
# CHECK: fmul q2, q3{{.*}}encoding: [0xff,0x2b]
# CHECK: fmul q3, q0{{.*}}encoding: [0xff,0x2c]
# CHECK: fmul q3, q1{{.*}}encoding: [0xff,0x2d]
# CHECK: fmul q3, q2{{.*}}encoding: [0xff,0x2e]
# CHECK: fmul q3, q3{{.*}}encoding: [0xff,0x2f]
# CHECK: fdiv q0, q0{{.*}}encoding: [0xff,0x30]
# CHECK: fdiv q0, q1{{.*}}encoding: [0xff,0x31]
# CHECK: fdiv q0, q2{{.*}}encoding: [0xff,0x32]
# CHECK: fdiv q0, q3{{.*}}encoding: [0xff,0x33]
# CHECK: fdiv q1, q0{{.*}}encoding: [0xff,0x34]
# CHECK: fdiv q1, q1{{.*}}encoding: [0xff,0x35]
# CHECK: fdiv q1, q2{{.*}}encoding: [0xff,0x36]
# CHECK: fdiv q1, q3{{.*}}encoding: [0xff,0x37]
# CHECK: fdiv q2, q0{{.*}}encoding: [0xff,0x38]
# CHECK: fdiv q2, q1{{.*}}encoding: [0xff,0x39]
# CHECK: fdiv q2, q2{{.*}}encoding: [0xff,0x3a]
# CHECK: fdiv q2, q3{{.*}}encoding: [0xff,0x3b]
# CHECK: fdiv q3, q0{{.*}}encoding: [0xff,0x3c]
# CHECK: fdiv q3, q1{{.*}}encoding: [0xff,0x3d]
# CHECK: fdiv q3, q2{{.*}}encoding: [0xff,0x3e]
# CHECK: fdiv q3, q3{{.*}}encoding: [0xff,0x3f]
# CHECK: fmin q0, q0{{.*}}encoding: [0xff,0x40]
# CHECK: fmin q0, q1{{.*}}encoding: [0xff,0x41]
# CHECK: fmin q0, q2{{.*}}encoding: [0xff,0x42]
# CHECK: fmin q0, q3{{.*}}encoding: [0xff,0x43]
# CHECK: fmin q1, q0{{.*}}encoding: [0xff,0x44]
# CHECK: fmin q1, q1{{.*}}encoding: [0xff,0x45]
# CHECK: fmin q1, q2{{.*}}encoding: [0xff,0x46]
# CHECK: fmin q1, q3{{.*}}encoding: [0xff,0x47]
# CHECK: fmin q2, q0{{.*}}encoding: [0xff,0x48]
# CHECK: fmin q2, q1{{.*}}encoding: [0xff,0x49]
# CHECK: fmin q2, q2{{.*}}encoding: [0xff,0x4a]
# CHECK: fmin q2, q3{{.*}}encoding: [0xff,0x4b]
# CHECK: fmin q3, q0{{.*}}encoding: [0xff,0x4c]
# CHECK: fmin q3, q1{{.*}}encoding: [0xff,0x4d]
# CHECK: fmin q3, q2{{.*}}encoding: [0xff,0x4e]
# CHECK: fmin q3, q3{{.*}}encoding: [0xff,0x4f]
# CHECK: fmax q0, q0{{.*}}encoding: [0xff,0x50]
# CHECK: fmax q0, q1{{.*}}encoding: [0xff,0x51]
# CHECK: fmax q0, q2{{.*}}encoding: [0xff,0x52]
# CHECK: fmax q0, q3{{.*}}encoding: [0xff,0x53]
# CHECK: fmax q1, q0{{.*}}encoding: [0xff,0x54]
# CHECK: fmax q1, q1{{.*}}encoding: [0xff,0x55]
# CHECK: fmax q1, q2{{.*}}encoding: [0xff,0x56]
# CHECK: fmax q1, q3{{.*}}encoding: [0xff,0x57]
# CHECK: fmax q2, q0{{.*}}encoding: [0xff,0x58]
# CHECK: fmax q2, q1{{.*}}encoding: [0xff,0x59]
# CHECK: fmax q2, q2{{.*}}encoding: [0xff,0x5a]
# CHECK: fmax q2, q3{{.*}}encoding: [0xff,0x5b]
# CHECK: fmax q3, q0{{.*}}encoding: [0xff,0x5c]
# CHECK: fmax q3, q1{{.*}}encoding: [0xff,0x5d]
# CHECK: fmax q3, q2{{.*}}encoding: [0xff,0x5e]
# CHECK: fmax q3, q3{{.*}}encoding: [0xff,0x5f]
# CHECK: fneg q0{{.*}}encoding: [0xff,0x60]
# CHECK: fneg q1{{.*}}encoding: [0xff,0x61]
# CHECK: fneg q2{{.*}}encoding: [0xff,0x62]
# CHECK: fneg q3{{.*}}encoding: [0xff,0x63]
# CHECK: fabs q0{{.*}}encoding: [0xff,0x64]
# CHECK: fabs q1{{.*}}encoding: [0xff,0x65]
# CHECK: fabs q2{{.*}}encoding: [0xff,0x66]
# CHECK: fabs q3{{.*}}encoding: [0xff,0x67]
# CHECK: fsqrt q0{{.*}}encoding: [0xff,0x68]
# CHECK: fsqrt q1{{.*}}encoding: [0xff,0x69]
# CHECK: fsqrt q2{{.*}}encoding: [0xff,0x6a]
# CHECK: fsqrt q3{{.*}}encoding: [0xff,0x6b]
# CHECK: ftrunc q0{{.*}}encoding: [0xff,0x6c]
# CHECK: ftrunc q1{{.*}}encoding: [0xff,0x6d]
# CHECK: ftrunc q2{{.*}}encoding: [0xff,0x6e]
# CHECK: ftrunc q3{{.*}}encoding: [0xff,0x6f]
# CHECK: ffloor q0{{.*}}encoding: [0xff,0x70]
# CHECK: ffloor q1{{.*}}encoding: [0xff,0x71]
# CHECK: ffloor q2{{.*}}encoding: [0xff,0x72]
# CHECK: ffloor q3{{.*}}encoding: [0xff,0x73]
# CHECK: fceil q0{{.*}}encoding: [0xff,0x74]
# CHECK: fceil q1{{.*}}encoding: [0xff,0x75]
# CHECK: fceil q2{{.*}}encoding: [0xff,0x76]
# CHECK: fceil q3{{.*}}encoding: [0xff,0x77]
# CHECK: fround q0{{.*}}encoding: [0xff,0x78]
# CHECK: fround q1{{.*}}encoding: [0xff,0x79]
# CHECK: fround q2{{.*}}encoding: [0xff,0x7a]
# CHECK: fround q3{{.*}}encoding: [0xff,0x7b]
# CHECK: s16tof q0, r0{{.*}}encoding: [0xff,0xc0,0x00]
# CHECK: s16tof q1, r0{{.*}}encoding: [0xff,0xc0,0x01]
# CHECK: s16tof q2, r0{{.*}}encoding: [0xff,0xc0,0x02]
# CHECK: s16tof q3, r0{{.*}}encoding: [0xff,0xc0,0x03]
# CHECK: s16tof q0, r1{{.*}}encoding: [0xff,0xc0,0x10]
# CHECK: s16tof q1, r1{{.*}}encoding: [0xff,0xc0,0x11]
# CHECK: s16tof q2, r1{{.*}}encoding: [0xff,0xc0,0x12]
# CHECK: s16tof q3, r1{{.*}}encoding: [0xff,0xc0,0x13]
# CHECK: s16tof q0, r2{{.*}}encoding: [0xff,0xc0,0x20]
# CHECK: s16tof q1, r2{{.*}}encoding: [0xff,0xc0,0x21]
# CHECK: s16tof q2, r2{{.*}}encoding: [0xff,0xc0,0x22]
# CHECK: s16tof q3, r2{{.*}}encoding: [0xff,0xc0,0x23]
# CHECK: s16tof q0, r3{{.*}}encoding: [0xff,0xc0,0x30]
# CHECK: s16tof q1, r3{{.*}}encoding: [0xff,0xc0,0x31]
# CHECK: s16tof q2, r3{{.*}}encoding: [0xff,0xc0,0x32]
# CHECK: s16tof q3, r3{{.*}}encoding: [0xff,0xc0,0x33]
# CHECK: s16tof q0, r4{{.*}}encoding: [0xff,0xc0,0x40]
# CHECK: s16tof q1, r4{{.*}}encoding: [0xff,0xc0,0x41]
# CHECK: s16tof q2, r4{{.*}}encoding: [0xff,0xc0,0x42]
# CHECK: s16tof q3, r4{{.*}}encoding: [0xff,0xc0,0x43]
# CHECK: s16tof q0, r5{{.*}}encoding: [0xff,0xc0,0x50]
# CHECK: s16tof q1, r5{{.*}}encoding: [0xff,0xc0,0x51]
# CHECK: s16tof q2, r5{{.*}}encoding: [0xff,0xc0,0x52]
# CHECK: s16tof q3, r5{{.*}}encoding: [0xff,0xc0,0x53]
# CHECK: s16tof q0, r6{{.*}}encoding: [0xff,0xc0,0x60]
# CHECK: s16tof q1, r6{{.*}}encoding: [0xff,0xc0,0x61]
# CHECK: s16tof q2, r6{{.*}}encoding: [0xff,0xc0,0x62]
# CHECK: s16tof q3, r6{{.*}}encoding: [0xff,0xc0,0x63]
# CHECK: s16tof q0, r7{{.*}}encoding: [0xff,0xc0,0x70]
# CHECK: s16tof q1, r7{{.*}}encoding: [0xff,0xc0,0x71]
# CHECK: s16tof q2, r7{{.*}}encoding: [0xff,0xc0,0x72]
# CHECK: s16tof q3, r7{{.*}}encoding: [0xff,0xc0,0x73]
# CHECK: u16tof q0, r0{{.*}}encoding: [0xff,0xc1,0x00]
# CHECK: u16tof q1, r0{{.*}}encoding: [0xff,0xc1,0x01]
# CHECK: u16tof q2, r0{{.*}}encoding: [0xff,0xc1,0x02]
# CHECK: u16tof q3, r0{{.*}}encoding: [0xff,0xc1,0x03]
# CHECK: u16tof q0, r1{{.*}}encoding: [0xff,0xc1,0x10]
# CHECK: u16tof q1, r1{{.*}}encoding: [0xff,0xc1,0x11]
# CHECK: u16tof q2, r1{{.*}}encoding: [0xff,0xc1,0x12]
# CHECK: u16tof q3, r1{{.*}}encoding: [0xff,0xc1,0x13]
# CHECK: u16tof q0, r2{{.*}}encoding: [0xff,0xc1,0x20]
# CHECK: u16tof q1, r2{{.*}}encoding: [0xff,0xc1,0x21]
# CHECK: u16tof q2, r2{{.*}}encoding: [0xff,0xc1,0x22]
# CHECK: u16tof q3, r2{{.*}}encoding: [0xff,0xc1,0x23]
# CHECK: u16tof q0, r3{{.*}}encoding: [0xff,0xc1,0x30]
# CHECK: u16tof q1, r3{{.*}}encoding: [0xff,0xc1,0x31]
# CHECK: u16tof q2, r3{{.*}}encoding: [0xff,0xc1,0x32]
# CHECK: u16tof q3, r3{{.*}}encoding: [0xff,0xc1,0x33]
# CHECK: u16tof q0, r4{{.*}}encoding: [0xff,0xc1,0x40]
# CHECK: u16tof q1, r4{{.*}}encoding: [0xff,0xc1,0x41]
# CHECK: u16tof q2, r4{{.*}}encoding: [0xff,0xc1,0x42]
# CHECK: u16tof q3, r4{{.*}}encoding: [0xff,0xc1,0x43]
# CHECK: u16tof q0, r5{{.*}}encoding: [0xff,0xc1,0x50]
# CHECK: u16tof q1, r5{{.*}}encoding: [0xff,0xc1,0x51]
# CHECK: u16tof q2, r5{{.*}}encoding: [0xff,0xc1,0x52]
# CHECK: u16tof q3, r5{{.*}}encoding: [0xff,0xc1,0x53]
# CHECK: u16tof q0, r6{{.*}}encoding: [0xff,0xc1,0x60]
# CHECK: u16tof q1, r6{{.*}}encoding: [0xff,0xc1,0x61]
# CHECK: u16tof q2, r6{{.*}}encoding: [0xff,0xc1,0x62]
# CHECK: u16tof q3, r6{{.*}}encoding: [0xff,0xc1,0x63]
# CHECK: u16tof q0, r7{{.*}}encoding: [0xff,0xc1,0x70]
# CHECK: u16tof q1, r7{{.*}}encoding: [0xff,0xc1,0x71]
# CHECK: u16tof q2, r7{{.*}}encoding: [0xff,0xc1,0x72]
# CHECK: u16tof q3, r7{{.*}}encoding: [0xff,0xc1,0x73]
# CHECK: ftos16 r0, q0{{.*}}encoding: [0xff,0xc2,0x00]
# CHECK: ftos16 r0, q1{{.*}}encoding: [0xff,0xc2,0x01]
# CHECK: ftos16 r0, q2{{.*}}encoding: [0xff,0xc2,0x02]
# CHECK: ftos16 r0, q3{{.*}}encoding: [0xff,0xc2,0x03]
# CHECK: ftos16 r1, q0{{.*}}encoding: [0xff,0xc2,0x10]
# CHECK: ftos16 r1, q1{{.*}}encoding: [0xff,0xc2,0x11]
# CHECK: ftos16 r1, q2{{.*}}encoding: [0xff,0xc2,0x12]
# CHECK: ftos16 r1, q3{{.*}}encoding: [0xff,0xc2,0x13]
# CHECK: ftos16 r2, q0{{.*}}encoding: [0xff,0xc2,0x20]
# CHECK: ftos16 r2, q1{{.*}}encoding: [0xff,0xc2,0x21]
# CHECK: ftos16 r2, q2{{.*}}encoding: [0xff,0xc2,0x22]
# CHECK: ftos16 r2, q3{{.*}}encoding: [0xff,0xc2,0x23]
# CHECK: ftos16 r3, q0{{.*}}encoding: [0xff,0xc2,0x30]
# CHECK: ftos16 r3, q1{{.*}}encoding: [0xff,0xc2,0x31]
# CHECK: ftos16 r3, q2{{.*}}encoding: [0xff,0xc2,0x32]
# CHECK: ftos16 r3, q3{{.*}}encoding: [0xff,0xc2,0x33]
# CHECK: ftos16 r4, q0{{.*}}encoding: [0xff,0xc2,0x40]
# CHECK: ftos16 r4, q1{{.*}}encoding: [0xff,0xc2,0x41]
# CHECK: ftos16 r4, q2{{.*}}encoding: [0xff,0xc2,0x42]
# CHECK: ftos16 r4, q3{{.*}}encoding: [0xff,0xc2,0x43]
# CHECK: ftos16 r5, q0{{.*}}encoding: [0xff,0xc2,0x50]
# CHECK: ftos16 r5, q1{{.*}}encoding: [0xff,0xc2,0x51]
# CHECK: ftos16 r5, q2{{.*}}encoding: [0xff,0xc2,0x52]
# CHECK: ftos16 r5, q3{{.*}}encoding: [0xff,0xc2,0x53]
# CHECK: ftos16 r6, q0{{.*}}encoding: [0xff,0xc2,0x60]
# CHECK: ftos16 r6, q1{{.*}}encoding: [0xff,0xc2,0x61]
# CHECK: ftos16 r6, q2{{.*}}encoding: [0xff,0xc2,0x62]
# CHECK: ftos16 r6, q3{{.*}}encoding: [0xff,0xc2,0x63]
# CHECK: ftos16 r7, q0{{.*}}encoding: [0xff,0xc2,0x70]
# CHECK: ftos16 r7, q1{{.*}}encoding: [0xff,0xc2,0x71]
# CHECK: ftos16 r7, q2{{.*}}encoding: [0xff,0xc2,0x72]
# CHECK: ftos16 r7, q3{{.*}}encoding: [0xff,0xc2,0x73]
# CHECK: ftou16 r0, q0{{.*}}encoding: [0xff,0xc3,0x00]
# CHECK: ftou16 r0, q1{{.*}}encoding: [0xff,0xc3,0x01]
# CHECK: ftou16 r0, q2{{.*}}encoding: [0xff,0xc3,0x02]
# CHECK: ftou16 r0, q3{{.*}}encoding: [0xff,0xc3,0x03]
# CHECK: ftou16 r1, q0{{.*}}encoding: [0xff,0xc3,0x10]
# CHECK: ftou16 r1, q1{{.*}}encoding: [0xff,0xc3,0x11]
# CHECK: ftou16 r1, q2{{.*}}encoding: [0xff,0xc3,0x12]
# CHECK: ftou16 r1, q3{{.*}}encoding: [0xff,0xc3,0x13]
# CHECK: ftou16 r2, q0{{.*}}encoding: [0xff,0xc3,0x20]
# CHECK: ftou16 r2, q1{{.*}}encoding: [0xff,0xc3,0x21]
# CHECK: ftou16 r2, q2{{.*}}encoding: [0xff,0xc3,0x22]
# CHECK: ftou16 r2, q3{{.*}}encoding: [0xff,0xc3,0x23]
# CHECK: ftou16 r3, q0{{.*}}encoding: [0xff,0xc3,0x30]
# CHECK: ftou16 r3, q1{{.*}}encoding: [0xff,0xc3,0x31]
# CHECK: ftou16 r3, q2{{.*}}encoding: [0xff,0xc3,0x32]
# CHECK: ftou16 r3, q3{{.*}}encoding: [0xff,0xc3,0x33]
# CHECK: ftou16 r4, q0{{.*}}encoding: [0xff,0xc3,0x40]
# CHECK: ftou16 r4, q1{{.*}}encoding: [0xff,0xc3,0x41]
# CHECK: ftou16 r4, q2{{.*}}encoding: [0xff,0xc3,0x42]
# CHECK: ftou16 r4, q3{{.*}}encoding: [0xff,0xc3,0x43]
# CHECK: ftou16 r5, q0{{.*}}encoding: [0xff,0xc3,0x50]
# CHECK: ftou16 r5, q1{{.*}}encoding: [0xff,0xc3,0x51]
# CHECK: ftou16 r5, q2{{.*}}encoding: [0xff,0xc3,0x52]
# CHECK: ftou16 r5, q3{{.*}}encoding: [0xff,0xc3,0x53]
# CHECK: ftou16 r6, q0{{.*}}encoding: [0xff,0xc3,0x60]
# CHECK: ftou16 r6, q1{{.*}}encoding: [0xff,0xc3,0x61]
# CHECK: ftou16 r6, q2{{.*}}encoding: [0xff,0xc3,0x62]
# CHECK: ftou16 r6, q3{{.*}}encoding: [0xff,0xc3,0x63]
# CHECK: ftou16 r7, q0{{.*}}encoding: [0xff,0xc3,0x70]
# CHECK: ftou16 r7, q1{{.*}}encoding: [0xff,0xc3,0x71]
# CHECK: ftou16 r7, q2{{.*}}encoding: [0xff,0xc3,0x72]
# CHECK: ftou16 r7, q3{{.*}}encoding: [0xff,0xc3,0x73]
# CHECK: fclass r0, q0{{.*}}encoding: [0xff,0xc9,0x00]
# CHECK: fclass r0, q1{{.*}}encoding: [0xff,0xc9,0x01]
# CHECK: fclass r0, q2{{.*}}encoding: [0xff,0xc9,0x02]
# CHECK: fclass r0, q3{{.*}}encoding: [0xff,0xc9,0x03]
# CHECK: fclass r1, q0{{.*}}encoding: [0xff,0xc9,0x10]
# CHECK: fclass r1, q1{{.*}}encoding: [0xff,0xc9,0x11]
# CHECK: fclass r1, q2{{.*}}encoding: [0xff,0xc9,0x12]
# CHECK: fclass r1, q3{{.*}}encoding: [0xff,0xc9,0x13]
# CHECK: fclass r2, q0{{.*}}encoding: [0xff,0xc9,0x20]
# CHECK: fclass r2, q1{{.*}}encoding: [0xff,0xc9,0x21]
# CHECK: fclass r2, q2{{.*}}encoding: [0xff,0xc9,0x22]
# CHECK: fclass r2, q3{{.*}}encoding: [0xff,0xc9,0x23]
# CHECK: fclass r3, q0{{.*}}encoding: [0xff,0xc9,0x30]
# CHECK: fclass r3, q1{{.*}}encoding: [0xff,0xc9,0x31]
# CHECK: fclass r3, q2{{.*}}encoding: [0xff,0xc9,0x32]
# CHECK: fclass r3, q3{{.*}}encoding: [0xff,0xc9,0x33]
# CHECK: fclass r4, q0{{.*}}encoding: [0xff,0xc9,0x40]
# CHECK: fclass r4, q1{{.*}}encoding: [0xff,0xc9,0x41]
# CHECK: fclass r4, q2{{.*}}encoding: [0xff,0xc9,0x42]
# CHECK: fclass r4, q3{{.*}}encoding: [0xff,0xc9,0x43]
# CHECK: fclass r5, q0{{.*}}encoding: [0xff,0xc9,0x50]
# CHECK: fclass r5, q1{{.*}}encoding: [0xff,0xc9,0x51]
# CHECK: fclass r5, q2{{.*}}encoding: [0xff,0xc9,0x52]
# CHECK: fclass r5, q3{{.*}}encoding: [0xff,0xc9,0x53]
# CHECK: fclass r6, q0{{.*}}encoding: [0xff,0xc9,0x60]
# CHECK: fclass r6, q1{{.*}}encoding: [0xff,0xc9,0x61]
# CHECK: fclass r6, q2{{.*}}encoding: [0xff,0xc9,0x62]
# CHECK: fclass r6, q3{{.*}}encoding: [0xff,0xc9,0x63]
# CHECK: fclass r7, q0{{.*}}encoding: [0xff,0xc9,0x70]
# CHECK: fclass r7, q1{{.*}}encoding: [0xff,0xc9,0x71]
# CHECK: fclass r7, q2{{.*}}encoding: [0xff,0xc9,0x72]
# CHECK: fclass r7, q3{{.*}}encoding: [0xff,0xc9,0x73]
# CHECK: s32tof q0, q0{{.*}}encoding: [0xff,0xc4,0x00]
# CHECK: s32tof q0, q1{{.*}}encoding: [0xff,0xc4,0x01]
# CHECK: s32tof q0, q2{{.*}}encoding: [0xff,0xc4,0x02]
# CHECK: s32tof q0, q3{{.*}}encoding: [0xff,0xc4,0x03]
# CHECK: s32tof q1, q0{{.*}}encoding: [0xff,0xc4,0x04]
# CHECK: s32tof q1, q1{{.*}}encoding: [0xff,0xc4,0x05]
# CHECK: s32tof q1, q2{{.*}}encoding: [0xff,0xc4,0x06]
# CHECK: s32tof q1, q3{{.*}}encoding: [0xff,0xc4,0x07]
# CHECK: s32tof q2, q0{{.*}}encoding: [0xff,0xc4,0x08]
# CHECK: s32tof q2, q1{{.*}}encoding: [0xff,0xc4,0x09]
# CHECK: s32tof q2, q2{{.*}}encoding: [0xff,0xc4,0x0a]
# CHECK: s32tof q2, q3{{.*}}encoding: [0xff,0xc4,0x0b]
# CHECK: s32tof q3, q0{{.*}}encoding: [0xff,0xc4,0x0c]
# CHECK: s32tof q3, q1{{.*}}encoding: [0xff,0xc4,0x0d]
# CHECK: s32tof q3, q2{{.*}}encoding: [0xff,0xc4,0x0e]
# CHECK: s32tof q3, q3{{.*}}encoding: [0xff,0xc4,0x0f]
# CHECK: u32tof q0, q0{{.*}}encoding: [0xff,0xc5,0x00]
# CHECK: u32tof q0, q1{{.*}}encoding: [0xff,0xc5,0x01]
# CHECK: u32tof q0, q2{{.*}}encoding: [0xff,0xc5,0x02]
# CHECK: u32tof q0, q3{{.*}}encoding: [0xff,0xc5,0x03]
# CHECK: u32tof q1, q0{{.*}}encoding: [0xff,0xc5,0x04]
# CHECK: u32tof q1, q1{{.*}}encoding: [0xff,0xc5,0x05]
# CHECK: u32tof q1, q2{{.*}}encoding: [0xff,0xc5,0x06]
# CHECK: u32tof q1, q3{{.*}}encoding: [0xff,0xc5,0x07]
# CHECK: u32tof q2, q0{{.*}}encoding: [0xff,0xc5,0x08]
# CHECK: u32tof q2, q1{{.*}}encoding: [0xff,0xc5,0x09]
# CHECK: u32tof q2, q2{{.*}}encoding: [0xff,0xc5,0x0a]
# CHECK: u32tof q2, q3{{.*}}encoding: [0xff,0xc5,0x0b]
# CHECK: u32tof q3, q0{{.*}}encoding: [0xff,0xc5,0x0c]
# CHECK: u32tof q3, q1{{.*}}encoding: [0xff,0xc5,0x0d]
# CHECK: u32tof q3, q2{{.*}}encoding: [0xff,0xc5,0x0e]
# CHECK: u32tof q3, q3{{.*}}encoding: [0xff,0xc5,0x0f]
# CHECK: ftos32 q0, q0{{.*}}encoding: [0xff,0xc6,0x00]
# CHECK: ftos32 q0, q1{{.*}}encoding: [0xff,0xc6,0x01]
# CHECK: ftos32 q0, q2{{.*}}encoding: [0xff,0xc6,0x02]
# CHECK: ftos32 q0, q3{{.*}}encoding: [0xff,0xc6,0x03]
# CHECK: ftos32 q1, q0{{.*}}encoding: [0xff,0xc6,0x04]
# CHECK: ftos32 q1, q1{{.*}}encoding: [0xff,0xc6,0x05]
# CHECK: ftos32 q1, q2{{.*}}encoding: [0xff,0xc6,0x06]
# CHECK: ftos32 q1, q3{{.*}}encoding: [0xff,0xc6,0x07]
# CHECK: ftos32 q2, q0{{.*}}encoding: [0xff,0xc6,0x08]
# CHECK: ftos32 q2, q1{{.*}}encoding: [0xff,0xc6,0x09]
# CHECK: ftos32 q2, q2{{.*}}encoding: [0xff,0xc6,0x0a]
# CHECK: ftos32 q2, q3{{.*}}encoding: [0xff,0xc6,0x0b]
# CHECK: ftos32 q3, q0{{.*}}encoding: [0xff,0xc6,0x0c]
# CHECK: ftos32 q3, q1{{.*}}encoding: [0xff,0xc6,0x0d]
# CHECK: ftos32 q3, q2{{.*}}encoding: [0xff,0xc6,0x0e]
# CHECK: ftos32 q3, q3{{.*}}encoding: [0xff,0xc6,0x0f]
# CHECK: ftou32 q0, q0{{.*}}encoding: [0xff,0xc7,0x00]
# CHECK: ftou32 q0, q1{{.*}}encoding: [0xff,0xc7,0x01]
# CHECK: ftou32 q0, q2{{.*}}encoding: [0xff,0xc7,0x02]
# CHECK: ftou32 q0, q3{{.*}}encoding: [0xff,0xc7,0x03]
# CHECK: ftou32 q1, q0{{.*}}encoding: [0xff,0xc7,0x04]
# CHECK: ftou32 q1, q1{{.*}}encoding: [0xff,0xc7,0x05]
# CHECK: ftou32 q1, q2{{.*}}encoding: [0xff,0xc7,0x06]
# CHECK: ftou32 q1, q3{{.*}}encoding: [0xff,0xc7,0x07]
# CHECK: ftou32 q2, q0{{.*}}encoding: [0xff,0xc7,0x08]
# CHECK: ftou32 q2, q1{{.*}}encoding: [0xff,0xc7,0x09]
# CHECK: ftou32 q2, q2{{.*}}encoding: [0xff,0xc7,0x0a]
# CHECK: ftou32 q2, q3{{.*}}encoding: [0xff,0xc7,0x0b]
# CHECK: ftou32 q3, q0{{.*}}encoding: [0xff,0xc7,0x0c]
# CHECK: ftou32 q3, q1{{.*}}encoding: [0xff,0xc7,0x0d]
# CHECK: ftou32 q3, q2{{.*}}encoding: [0xff,0xc7,0x0e]
# CHECK: ftou32 q3, q3{{.*}}encoding: [0xff,0xc7,0x0f]
# CHECK: fcmp r0, q0, q0{{.*}}encoding: [0xff,0xc8,0x00]
# CHECK: fcmp r0, q0, q1{{.*}}encoding: [0xff,0xc8,0x01]
# CHECK: fcmp r0, q0, q2{{.*}}encoding: [0xff,0xc8,0x02]
# CHECK: fcmp r0, q0, q3{{.*}}encoding: [0xff,0xc8,0x03]
# CHECK: fcmp r0, q1, q0{{.*}}encoding: [0xff,0xc8,0x04]
# CHECK: fcmp r0, q1, q1{{.*}}encoding: [0xff,0xc8,0x05]
# CHECK: fcmp r0, q1, q2{{.*}}encoding: [0xff,0xc8,0x06]
# CHECK: fcmp r0, q1, q3{{.*}}encoding: [0xff,0xc8,0x07]
# CHECK: fcmp r0, q2, q0{{.*}}encoding: [0xff,0xc8,0x08]
# CHECK: fcmp r0, q2, q1{{.*}}encoding: [0xff,0xc8,0x09]
# CHECK: fcmp r0, q2, q2{{.*}}encoding: [0xff,0xc8,0x0a]
# CHECK: fcmp r0, q2, q3{{.*}}encoding: [0xff,0xc8,0x0b]
# CHECK: fcmp r0, q3, q0{{.*}}encoding: [0xff,0xc8,0x0c]
# CHECK: fcmp r0, q3, q1{{.*}}encoding: [0xff,0xc8,0x0d]
# CHECK: fcmp r0, q3, q2{{.*}}encoding: [0xff,0xc8,0x0e]
# CHECK: fcmp r0, q3, q3{{.*}}encoding: [0xff,0xc8,0x0f]
# CHECK: fcmp r1, q0, q0{{.*}}encoding: [0xff,0xc8,0x10]
# CHECK: fcmp r1, q0, q1{{.*}}encoding: [0xff,0xc8,0x11]
# CHECK: fcmp r1, q0, q2{{.*}}encoding: [0xff,0xc8,0x12]
# CHECK: fcmp r1, q0, q3{{.*}}encoding: [0xff,0xc8,0x13]
# CHECK: fcmp r1, q1, q0{{.*}}encoding: [0xff,0xc8,0x14]
# CHECK: fcmp r1, q1, q1{{.*}}encoding: [0xff,0xc8,0x15]
# CHECK: fcmp r1, q1, q2{{.*}}encoding: [0xff,0xc8,0x16]
# CHECK: fcmp r1, q1, q3{{.*}}encoding: [0xff,0xc8,0x17]
# CHECK: fcmp r1, q2, q0{{.*}}encoding: [0xff,0xc8,0x18]
# CHECK: fcmp r1, q2, q1{{.*}}encoding: [0xff,0xc8,0x19]
# CHECK: fcmp r1, q2, q2{{.*}}encoding: [0xff,0xc8,0x1a]
# CHECK: fcmp r1, q2, q3{{.*}}encoding: [0xff,0xc8,0x1b]
# CHECK: fcmp r1, q3, q0{{.*}}encoding: [0xff,0xc8,0x1c]
# CHECK: fcmp r1, q3, q1{{.*}}encoding: [0xff,0xc8,0x1d]
# CHECK: fcmp r1, q3, q2{{.*}}encoding: [0xff,0xc8,0x1e]
# CHECK: fcmp r1, q3, q3{{.*}}encoding: [0xff,0xc8,0x1f]
# CHECK: fcmp r2, q0, q0{{.*}}encoding: [0xff,0xc8,0x20]
# CHECK: fcmp r2, q0, q1{{.*}}encoding: [0xff,0xc8,0x21]
# CHECK: fcmp r2, q0, q2{{.*}}encoding: [0xff,0xc8,0x22]
# CHECK: fcmp r2, q0, q3{{.*}}encoding: [0xff,0xc8,0x23]
# CHECK: fcmp r2, q1, q0{{.*}}encoding: [0xff,0xc8,0x24]
# CHECK: fcmp r2, q1, q1{{.*}}encoding: [0xff,0xc8,0x25]
# CHECK: fcmp r2, q1, q2{{.*}}encoding: [0xff,0xc8,0x26]
# CHECK: fcmp r2, q1, q3{{.*}}encoding: [0xff,0xc8,0x27]
# CHECK: fcmp r2, q2, q0{{.*}}encoding: [0xff,0xc8,0x28]
# CHECK: fcmp r2, q2, q1{{.*}}encoding: [0xff,0xc8,0x29]
# CHECK: fcmp r2, q2, q2{{.*}}encoding: [0xff,0xc8,0x2a]
# CHECK: fcmp r2, q2, q3{{.*}}encoding: [0xff,0xc8,0x2b]
# CHECK: fcmp r2, q3, q0{{.*}}encoding: [0xff,0xc8,0x2c]
# CHECK: fcmp r2, q3, q1{{.*}}encoding: [0xff,0xc8,0x2d]
# CHECK: fcmp r2, q3, q2{{.*}}encoding: [0xff,0xc8,0x2e]
# CHECK: fcmp r2, q3, q3{{.*}}encoding: [0xff,0xc8,0x2f]
# CHECK: fcmp r3, q0, q0{{.*}}encoding: [0xff,0xc8,0x30]
# CHECK: fcmp r3, q0, q1{{.*}}encoding: [0xff,0xc8,0x31]
# CHECK: fcmp r3, q0, q2{{.*}}encoding: [0xff,0xc8,0x32]
# CHECK: fcmp r3, q0, q3{{.*}}encoding: [0xff,0xc8,0x33]
# CHECK: fcmp r3, q1, q0{{.*}}encoding: [0xff,0xc8,0x34]
# CHECK: fcmp r3, q1, q1{{.*}}encoding: [0xff,0xc8,0x35]
# CHECK: fcmp r3, q1, q2{{.*}}encoding: [0xff,0xc8,0x36]
# CHECK: fcmp r3, q1, q3{{.*}}encoding: [0xff,0xc8,0x37]
# CHECK: fcmp r3, q2, q0{{.*}}encoding: [0xff,0xc8,0x38]
# CHECK: fcmp r3, q2, q1{{.*}}encoding: [0xff,0xc8,0x39]
# CHECK: fcmp r3, q2, q2{{.*}}encoding: [0xff,0xc8,0x3a]
# CHECK: fcmp r3, q2, q3{{.*}}encoding: [0xff,0xc8,0x3b]
# CHECK: fcmp r3, q3, q0{{.*}}encoding: [0xff,0xc8,0x3c]
# CHECK: fcmp r3, q3, q1{{.*}}encoding: [0xff,0xc8,0x3d]
# CHECK: fcmp r3, q3, q2{{.*}}encoding: [0xff,0xc8,0x3e]
# CHECK: fcmp r3, q3, q3{{.*}}encoding: [0xff,0xc8,0x3f]
# CHECK: fcmp r4, q0, q0{{.*}}encoding: [0xff,0xc8,0x40]
# CHECK: fcmp r4, q0, q1{{.*}}encoding: [0xff,0xc8,0x41]
# CHECK: fcmp r4, q0, q2{{.*}}encoding: [0xff,0xc8,0x42]
# CHECK: fcmp r4, q0, q3{{.*}}encoding: [0xff,0xc8,0x43]
# CHECK: fcmp r4, q1, q0{{.*}}encoding: [0xff,0xc8,0x44]
# CHECK: fcmp r4, q1, q1{{.*}}encoding: [0xff,0xc8,0x45]
# CHECK: fcmp r4, q1, q2{{.*}}encoding: [0xff,0xc8,0x46]
# CHECK: fcmp r4, q1, q3{{.*}}encoding: [0xff,0xc8,0x47]
# CHECK: fcmp r4, q2, q0{{.*}}encoding: [0xff,0xc8,0x48]
# CHECK: fcmp r4, q2, q1{{.*}}encoding: [0xff,0xc8,0x49]
# CHECK: fcmp r4, q2, q2{{.*}}encoding: [0xff,0xc8,0x4a]
# CHECK: fcmp r4, q2, q3{{.*}}encoding: [0xff,0xc8,0x4b]
# CHECK: fcmp r4, q3, q0{{.*}}encoding: [0xff,0xc8,0x4c]
# CHECK: fcmp r4, q3, q1{{.*}}encoding: [0xff,0xc8,0x4d]
# CHECK: fcmp r4, q3, q2{{.*}}encoding: [0xff,0xc8,0x4e]
# CHECK: fcmp r4, q3, q3{{.*}}encoding: [0xff,0xc8,0x4f]
# CHECK: fcmp r5, q0, q0{{.*}}encoding: [0xff,0xc8,0x50]
# CHECK: fcmp r5, q0, q1{{.*}}encoding: [0xff,0xc8,0x51]
# CHECK: fcmp r5, q0, q2{{.*}}encoding: [0xff,0xc8,0x52]
# CHECK: fcmp r5, q0, q3{{.*}}encoding: [0xff,0xc8,0x53]
# CHECK: fcmp r5, q1, q0{{.*}}encoding: [0xff,0xc8,0x54]
# CHECK: fcmp r5, q1, q1{{.*}}encoding: [0xff,0xc8,0x55]
# CHECK: fcmp r5, q1, q2{{.*}}encoding: [0xff,0xc8,0x56]
# CHECK: fcmp r5, q1, q3{{.*}}encoding: [0xff,0xc8,0x57]
# CHECK: fcmp r5, q2, q0{{.*}}encoding: [0xff,0xc8,0x58]
# CHECK: fcmp r5, q2, q1{{.*}}encoding: [0xff,0xc8,0x59]
# CHECK: fcmp r5, q2, q2{{.*}}encoding: [0xff,0xc8,0x5a]
# CHECK: fcmp r5, q2, q3{{.*}}encoding: [0xff,0xc8,0x5b]
# CHECK: fcmp r5, q3, q0{{.*}}encoding: [0xff,0xc8,0x5c]
# CHECK: fcmp r5, q3, q1{{.*}}encoding: [0xff,0xc8,0x5d]
# CHECK: fcmp r5, q3, q2{{.*}}encoding: [0xff,0xc8,0x5e]
# CHECK: fcmp r5, q3, q3{{.*}}encoding: [0xff,0xc8,0x5f]
# CHECK: fcmp r6, q0, q0{{.*}}encoding: [0xff,0xc8,0x60]
# CHECK: fcmp r6, q0, q1{{.*}}encoding: [0xff,0xc8,0x61]
# CHECK: fcmp r6, q0, q2{{.*}}encoding: [0xff,0xc8,0x62]
# CHECK: fcmp r6, q0, q3{{.*}}encoding: [0xff,0xc8,0x63]
# CHECK: fcmp r6, q1, q0{{.*}}encoding: [0xff,0xc8,0x64]
# CHECK: fcmp r6, q1, q1{{.*}}encoding: [0xff,0xc8,0x65]
# CHECK: fcmp r6, q1, q2{{.*}}encoding: [0xff,0xc8,0x66]
# CHECK: fcmp r6, q1, q3{{.*}}encoding: [0xff,0xc8,0x67]
# CHECK: fcmp r6, q2, q0{{.*}}encoding: [0xff,0xc8,0x68]
# CHECK: fcmp r6, q2, q1{{.*}}encoding: [0xff,0xc8,0x69]
# CHECK: fcmp r6, q2, q2{{.*}}encoding: [0xff,0xc8,0x6a]
# CHECK: fcmp r6, q2, q3{{.*}}encoding: [0xff,0xc8,0x6b]
# CHECK: fcmp r6, q3, q0{{.*}}encoding: [0xff,0xc8,0x6c]
# CHECK: fcmp r6, q3, q1{{.*}}encoding: [0xff,0xc8,0x6d]
# CHECK: fcmp r6, q3, q2{{.*}}encoding: [0xff,0xc8,0x6e]
# CHECK: fcmp r6, q3, q3{{.*}}encoding: [0xff,0xc8,0x6f]
# CHECK: fcmp r7, q0, q0{{.*}}encoding: [0xff,0xc8,0x70]
# CHECK: fcmp r7, q0, q1{{.*}}encoding: [0xff,0xc8,0x71]
# CHECK: fcmp r7, q0, q2{{.*}}encoding: [0xff,0xc8,0x72]
# CHECK: fcmp r7, q0, q3{{.*}}encoding: [0xff,0xc8,0x73]
# CHECK: fcmp r7, q1, q0{{.*}}encoding: [0xff,0xc8,0x74]
# CHECK: fcmp r7, q1, q1{{.*}}encoding: [0xff,0xc8,0x75]
# CHECK: fcmp r7, q1, q2{{.*}}encoding: [0xff,0xc8,0x76]
# CHECK: fcmp r7, q1, q3{{.*}}encoding: [0xff,0xc8,0x77]
# CHECK: fcmp r7, q2, q0{{.*}}encoding: [0xff,0xc8,0x78]
# CHECK: fcmp r7, q2, q1{{.*}}encoding: [0xff,0xc8,0x79]
# CHECK: fcmp r7, q2, q2{{.*}}encoding: [0xff,0xc8,0x7a]
# CHECK: fcmp r7, q2, q3{{.*}}encoding: [0xff,0xc8,0x7b]
# CHECK: fcmp r7, q3, q0{{.*}}encoding: [0xff,0xc8,0x7c]
# CHECK: fcmp r7, q3, q1{{.*}}encoding: [0xff,0xc8,0x7d]
# CHECK: fcmp r7, q3, q2{{.*}}encoding: [0xff,0xc8,0x7e]
# CHECK: fcmp r7, q3, q3{{.*}}encoding: [0xff,0xc8,0x7f]
