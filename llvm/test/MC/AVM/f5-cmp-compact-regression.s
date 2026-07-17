# RUN: llvm-mc -triple=avm --show-encoding %s | FileCheck %s

cmp c0,c0
cmp c0,c1
cmp c1,c2
cmp c3,c3
cmp r0,r1
cmp r4,r1
cmp r7,r3
CMP R4,R2
CmP r7,R3

# CHECK: cmp r4, r4{{.*}}encoding: [0x30]
# CHECK: cmp r4, r5{{.*}}encoding: [0x31]
# CHECK: cmp r5, r6{{.*}}encoding: [0x36]
# CHECK: cmp r7, r7{{.*}}encoding: [0x3f]
# CHECK: cmp r0, r1{{.*}}encoding: [0xf5,0x01]
# CHECK: cmp r4, r1{{.*}}encoding: [0xf5,0x21]
# CHECK: cmp r7, r3{{.*}}encoding: [0xf5,0x2f]
# CHECK: cmp r4, r2{{.*}}encoding: [0xf5,0x22]
# CHECK: cmp r7, r3{{.*}}encoding: [0xf5,0x2f]

