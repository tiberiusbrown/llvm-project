# RUN: llvm-mc -triple=avm --show-encoding %s | FileCheck %s

UDIV16 R3,R3
UrEm16 r3,R3
sDiV16 R3,r3
SREM16 r3,r3

# CHECK: udiv16 r3, r3{{.*}}encoding: [0xec,0x1b]
# CHECK: urem16 r3, r3{{.*}}encoding: [0xec,0x5b]
# CHECK: sdiv16 r3, r3{{.*}}encoding: [0xec,0x9b]
# CHECK: srem16 r3, r3{{.*}}encoding: [0xec,0xdb]
