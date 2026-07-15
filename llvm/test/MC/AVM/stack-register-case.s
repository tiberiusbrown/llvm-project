# RUN: llvm-mc -triple=avm-unknown-arduboyfx -show-encoding %s | FileCheck %s

PuSh16 R0
pOp16 r7

# CHECK: push16{{ *}}r0{{.*}}encoding: [0xb0]
# CHECK: pop16{{ *}}r7{{.*}}encoding: [0xbf]
