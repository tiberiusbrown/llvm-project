# RUN: not llvm-mc -triple=avm --show-encoding %s 2>&1 | FileCheck %s

mov32 r0, q0
mov32 q0, r0
mov32 q0
mov32 q0 q1
mov32 q0, q1, q2
fadd q2, q3
fadd q0, r0
fadd q0
fneg r0
fneg q0, q1
s16tof q2, r4
s16tof q0, q1
ftos16 r4, q2
s32tof r0, q0
fcmp r6, q2, q3
fcmp r0, r1, q2
fcmp r0, q1
fclass r6, q2
fclass r0, r1
fsqrt q2
ftrunc q2
ffloor q2
fceil q2
fround q2
fmod q2
frempio2 q2
fma q2
# CHECK-COUNT-27: error:
