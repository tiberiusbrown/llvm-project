# RUN: not llvm-mc -triple=avm -show-encoding %s 2>&1 | FileCheck %s

ldp8u q0,[q1]
ldp24 r0,[q1]
ldp16 c0,[q1]
ldp32 q0,[r1]
ldp8s r0,[q1+]
ldp24 q0,[q0+1]
ldp8u r0,q1
ldp8u r0
ldp16 r0,[q0],r1
ldp24 [q0],q1

# CHECK: error:
