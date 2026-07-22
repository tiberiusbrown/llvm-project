# RUN: not llvm-mc -triple=avm -filetype=null %s 2>&1 | FileCheck %s

st8 [q0+], r1
st8 [sp+], r1
st8 [pc+], r1
st8 [cc+], r1
st8 [c0+], c0
st8 [c0+], q0
st8 [c0+], sp
st8 [c0+], pc
st8 [c0+], cc
st8 [c0-], r0
st8 [c0++], r0
st8 c0, r0
st8 r0, [c0+]
st8
st8 [c0+]
st8 [c0+], r0, r1

# CHECK-COUNT-15: error:
