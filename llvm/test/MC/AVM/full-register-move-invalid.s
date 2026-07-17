# RUN: not llvm-mc -triple=avm -filetype=null %s 2>&1 | FileCheck %s

mov c0, r0
mov r0, c0
mov q0, r0
mov r0, q0
mov sp, r0
mov r0, sp
mov r0, cc
mov r4, r4
mov r4, r7
mov r5, r6
mov r7, r4
mov r0, 1
mov 1, r0
mov r0, [r1]
mov [r0], r1
mov r0, symbol
mov
mov r0
mov r0, r1, r2

# CHECK: error: expected full register r0-r7
# CHECK: error: expected full register r0-r7
# CHECK: error: expected full register r0-r7
# CHECK: error: expected full register r0-r7
# CHECK: error: expected full register r0-r7
# CHECK: error: expected AVM register
# CHECK: error: expected AVM register
# CHECK: error: expected AVM register
# CHECK: error: expected comma
# CHECK: error: unexpected token after AVM instruction
