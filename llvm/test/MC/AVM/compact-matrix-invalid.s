# RUN: not llvm-mc -triple=avm-unknown-arduboyfx -filetype=null %s 2>&1 | FileCheck %s

mov r0,c0
mov c0,r0
mov q0,c0
mov sp,c0
mov c0
mov c0,c1,c2

ld8u c0,c1
ld8u c0,[r0]
ld8u c0,[q0]
ld8u c0,[c0+]
ld8u [c0],c1

st8 c0,c1
st8 [r0],c0
st8 [c0+],c0
st8 [c0]

clr r0
clr q0
clr
nop c0

# CHECK: error: expected full register r0-r7
# CHECK: error: expected compact register c0-c3
# CHECK: error: expected compact register c0-c3
# CHECK: error: expected compact register c0-c3
# CHECK: error: expected comma
# CHECK: error: unexpected token after AVM instruction
# CHECK: error: expected compact memory operand '[cN]'
# CHECK: error: expected compact register c0-c3
# CHECK: error: expected compact register c0-c3
# CHECK: error: postincrement memory operands are not supported
# CHECK: error: expected AVM register
# CHECK: error: expected compact memory operand '[cN]'
# CHECK: error: expected compact register c0-c3
# CHECK: error: postincrement memory operands are not supported
# CHECK: error: expected comma
# CHECK: error: expected compact register c0-c3
# CHECK: error: expected compact register c0-c3
# CHECK: error: expected AVM register
# CHECK: error: unexpected token after AVM instruction
