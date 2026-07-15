# RUN: llvm-mc -triple=avm -show-encoding %s | FileCheck %s

ret
zext8 r0
mov r0, r1
getsp r7
jmpf 0x123456
setsp r4

# CHECK: ret{{.*}}encoding: [0xef]
# CHECK: zext8 r0{{.*}}encoding: [0xf1,0x70]
# CHECK: mov r0, r1{{.*}}encoding: [0xf1,0x01]
# CHECK: getsp r7{{.*}}encoding: [0xf1,0x87]
# CHECK: jmpf{{.*}}encoding: [0xe2,0x56,0x34,0x12]
# CHECK: setsp r4{{.*}}encoding: [0xf1,0x8c]
