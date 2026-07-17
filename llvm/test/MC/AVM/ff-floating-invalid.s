# RUN: not llvm-mc -triple=avm --show-encoding %s 2>&1 | FileCheck %s

mov32 r0, q0
mov32 q0, r0
mov32 q0
mov32 q0 q1
mov32 q0, q1, q2
fadd r0, q0
fadd q0, r0
fadd q0
fneg r0
fneg q0, q1
s16tof r0, r1
s16tof q0, q1
ftos16 q0, r0
s32tof r0, q0
fcmp q0, q1, q2
fcmp r0, r1, q2
fcmp r0, q1
fclass q0, q1
fclass r0, r1
# CHECK-COUNT-19: error:
