# RUN: not llvm-mc -triple=avm -show-encoding %s 2>&1 | FileCheck %s

# Every prohibited postincrement overlap (16 scalar plus 8 pair) is rejected.
ldp8u r0, [q0+]
ldp8u r1, [q0+]
ldp8u r2, [q1+]
ldp8u r3, [q1+]
ldp8u r4, [q2+]
ldp8u r5, [q2+]
ldp8u r6, [q3+]
ldp8u r7, [q3+]
ldp16 r0, [q0+]
ldp16 r1, [q0+]
ldp16 r2, [q1+]
ldp16 r3, [q1+]
ldp16 r4, [q2+]
ldp16 r5, [q2+]
ldp16 r6, [q3+]
ldp16 r7, [q3+]
ldp24 q0, [q0+]
ldp24 q1, [q1+]
ldp24 q2, [q2+]
ldp24 q3, [q3+]
ldp32 q0, [q0+]
ldp32 q1, [q1+]
ldp32 q2, [q2+]
ldp32 q3, [q3+]

# CHECK-COUNT-24: error: postincrement destination must not overlap address pair
