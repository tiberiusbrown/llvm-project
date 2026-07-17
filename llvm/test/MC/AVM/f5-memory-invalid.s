# RUN: not llvm-mc -triple=avm -filetype=null %s 2>&1 | FileCheck %s
ld8u r4,[c0]
ld8u r7,[c3]
ld8u q0,[c0]
ld8u sp,[c0]
ld8u r0,[q0]
ld8u r0,[c0+]
ld8u [c0],r0
ld8u r0
ld16 r4,[c0]
ld16 r7,[c3]
ld16 q0,[c0]
ld16 pc,[c0]
ld16 r0,[sp+0]
ld16 [c0],r0
st16 [c0],r4
st16 [c3],r7
st16 [c0],q0
st16 [c0],sp
st16 [q0],r0
st16 r0,[c0]
st16 [c0]
st16 [c0],r0,r1

# CHECK-COUNT-15: error:
