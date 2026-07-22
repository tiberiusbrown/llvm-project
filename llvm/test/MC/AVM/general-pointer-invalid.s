# RUN: not llvm-mc -triple=avm -show-encoding %s 2>&1 | FileCheck %s

# The 16 architecturally prohibited postincrement-load aliases.
ld8u r0, [r0+]
ld8u r1, [r1+]
ld8u r2, [r2+]
ld8u r3, [r3+]
ld8u r4, [r4+]
ld8u r5, [r5+]
ld8u r6, [r6+]
ld8u r7, [r7+]
ld16 r0, [r0+]
ld16 r1, [r1+]
ld16 r2, [r2+]
ld16 r3, [r3+]
ld16 r4, [r4+]
ld16 r5, [r5+]
ld16 r6, [r6+]
ld16 r7, [r7+]

# Wrong register classes, memory syntax, order, and operand counts.
ld8u c0, [r1]
ld8u q0, [r1]
ld8u r0, [q1]
ld16 r0, [sp+0]
st8 [r0], c0
st8 [q0], r0
st16 [sp+0], r0
ld8u r0, r1
st8 r0, r1
ld8u [r0], r1
ld16 [r0+], r1
st8 r1, [r0]
st16 r1, [r0+]
ld8u r0
st8 [r0]

# CHECK-COUNT-16: error: postincrement destination must not overlap address register
# CHECK: error:
