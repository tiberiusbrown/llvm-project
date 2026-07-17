# RUN: not llvm-mc -triple=avm -filetype=null %s 2>&1 | FileCheck %s
# CHECK-COUNT-24: error:

udiv16
udiv16 r0, r1, r2
udiv16 q0, r1
udiv16 sp, r1
udiv16 r8, r1
udiv16 r0, 1
urem16
urem16 r0, r1, r2
urem16 q0, r1
urem16 sp, r1
urem16 r8, r1
urem16 r0, 1
sdiv16
sdiv16 r0, r1, r2
sdiv16 q0, r1
sdiv16 sp, r1
sdiv16 r8, r1
sdiv16 r0, 1
srem16
srem16 r0, r1, r2
srem16 q0, r1
srem16 sp, r1
srem16 r8, r1
srem16 r0, 1
