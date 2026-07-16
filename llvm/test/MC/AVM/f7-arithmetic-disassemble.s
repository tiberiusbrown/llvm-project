# RUN: llvm-mc -triple=avm --disassemble < %S/Inputs/f7-arithmetic-valid.txt | FileCheck %s
# CHECK: add32 q0, q0
# CHECK: add32 q0, q1
# CHECK: add32 q0, q2
# CHECK: add32 q0, q3
# CHECK: add32 q1, q0
# CHECK: add32 q1, q1
# CHECK: add32 q1, q2
# CHECK: add32 q1, q3
# CHECK: add32 q2, q0
# CHECK: add32 q2, q1
# CHECK: add32 q2, q2
# CHECK: add32 q2, q3
# CHECK: add32 q3, q0
# CHECK: add32 q3, q1
# CHECK: add32 q3, q2
# CHECK: add32 q3, q3
# CHECK: sub32 q0, q0
# CHECK: sub32 q0, q1
# CHECK: sub32 q0, q2
# CHECK: sub32 q0, q3
# CHECK: sub32 q1, q0
# CHECK: sub32 q1, q1
# CHECK: sub32 q1, q2
# CHECK: sub32 q1, q3
# CHECK: sub32 q2, q0
# CHECK: sub32 q2, q1
# CHECK: sub32 q2, q2
# CHECK: sub32 q2, q3
# CHECK: sub32 q3, q0
# CHECK: sub32 q3, q1
# CHECK: sub32 q3, q2
# CHECK: sub32 q3, q3
# CHECK: lsr32.1 q0
# CHECK: lsr32.1 q1
# CHECK: lsr32.1 q2
# CHECK: lsr32.1 q3
# CHECK: asr32.1 q0
# CHECK: asr32.1 q1
# CHECK: asr32.1 q2
# CHECK: asr32.1 q3
# CHECK: bool r0
# CHECK: bool r1
# CHECK: bool r2
# CHECK: bool r3
# CHECK: bool r4
# CHECK: bool r5
# CHECK: bool r6
# CHECK: bool r7
