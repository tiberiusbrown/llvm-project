# RUN: not llvm-mc -triple=avm-unknown-arduboyfx %s 2>&1 | FileCheck %s

push16 c0
push16 q0
push16 sp
push16 pc
push16 cc
push16 0
push16 [r0]
push16
pop16 c0
pop16 q0
pop16 sp
pop16 pc
pop16 cc
pop16 0
pop16 [r0]
pop16

push16 r0, r1
pop16 r0, r1

# CHECK-COUNT-8: error: expected full register r0-r7
# CHECK-COUNT-2: error: unexpected token after AVM instruction
