# RUN: not llvm-mc -triple=avm -show-encoding %s 2>&1 | FileCheck %s
cmp32 r0,q0
cmp32 q0,r0
cmp32 c0,q0
ld32 r0,[r1]
ld32 q0,[c0]
ld32 q0,[q1]
ld32 q0,[r1+]
st32 [r0],r1
st32 [c0],q0
st32 [q0],q0
st32 [r0+],q0
ld32 [r0],q0
st32 q0,[r0]
cmp32 q0
cmp32 q0,q1,q2
ld32 q0
ld32 q0,[r1],q2
st32 [r0]
st32 [r0],q0,q1
cmp32 q0,123
ld32 q0,[symbol]
st32 [symbol],q0
# CHECK: error:
