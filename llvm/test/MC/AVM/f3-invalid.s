# RUN: not llvm-mc -triple=avm-unknown-arduboyfx %s 2>&1 | FileCheck %s

# CHECK: error:
st8 [c0], r4
st8 [c0], q0
st8 [r0], r1
st8 [q0], r1
st8 [c0+], r1
st8 r1, [c0]
st8 [c0]
st8 [c0], r0, r1
mulu8.w r4, c0
mulu8.w c0, r4
mulu8.w q0, c0
mulu8.w c0, 1
mulu8.w c0, [c1]
muls8.w r4, c0
mulsu8.w c0, r4
mulu8.w c0
muls8.w c0, c1, c2
mulsu8.w c0, c1, c2
