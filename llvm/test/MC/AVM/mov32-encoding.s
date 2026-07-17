# RUN: llvm-mc -triple=avm --show-encoding %s | FileCheck %s

# All source-level mov32 forms are exactly two bytes.
mov32 q0, q0
mov32 q0, q1
mov32 q0, q2
mov32 q0, q3
mov32 q1, q0
mov32 q1, q1
mov32 q1, q2
mov32 q1, q3
mov32 q2, q0
mov32 q2, q1
mov32 q2, q2
mov32 q2, q3
mov32 q3, q0
mov32 q3, q1
mov32 q3, q2
mov32 q3, q3

# CHECK: mov32 q0, q0{{.*}}encoding: [0xf2,0x60]
# CHECK: mov32 q0, q1{{.*}}encoding: [0xf2,0x61]
# CHECK: mov32 q0, q2{{.*}}encoding: [0xf2,0x62]
# CHECK: mov32 q0, q3{{.*}}encoding: [0xf2,0x63]
# CHECK: mov32 q1, q0{{.*}}encoding: [0xf2,0x64]
# CHECK: mov32 q1, q1{{.*}}encoding: [0xf2,0x65]
# CHECK: mov32 q1, q2{{.*}}encoding: [0xf2,0x66]
# CHECK: mov32 q1, q3{{.*}}encoding: [0xf2,0x67]
# CHECK: mov32 q2, q0{{.*}}encoding: [0xf2,0x68]
# CHECK: mov32 q2, q1{{.*}}encoding: [0xf2,0x69]
# CHECK: mov32 q2, q2{{.*}}encoding: [0x00,0x05]
# CHECK: mov32 q2, q3{{.*}}encoding: [0x02,0x07]
# CHECK: mov32 q3, q0{{.*}}encoding: [0xf2,0x6a]
# CHECK: mov32 q3, q1{{.*}}encoding: [0xf2,0x6b]
# CHECK: mov32 q3, q2{{.*}}encoding: [0x08,0x0d]
# CHECK: mov32 q3, q3{{.*}}encoding: [0x0a,0x0f]
