# RUN: not llvm-mc -triple=avm-unknown-arduboyfx -filetype=obj %s -o %t.o 2>&1 | FileCheck %s

mov32 q4, q0
add32 r0, q1
cmp32 q2, r6

# CHECK: error: expected AVM register pair q0-q3
# CHECK: error: expected AVM register pair q0-q3
# CHECK: error: expected AVM register pair q0-q3
