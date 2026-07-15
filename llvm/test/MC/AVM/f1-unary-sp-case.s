# RUN: llvm-mc -triple=avm -show-encoding %s | FileCheck %s

SWAP8 R7
GetSp r3
SeTsP R5
zExT8 R4

# CHECK: swap8 r7{{.*}}encoding: [0xf1,0x7f]
# CHECK: getsp r3{{.*}}encoding: [0xf1,0x83]
# CHECK: setsp r5{{.*}}encoding: [0xf1,0x8d]
# CHECK: zext8 r4{{.*}}encoding: [0xf1,0x74]
