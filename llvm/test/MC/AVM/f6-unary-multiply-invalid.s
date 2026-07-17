# RUN: not llvm-mc -triple=avm -filetype=null %s 2>&1 | FileCheck %s
bswap16 c0
bswap16 q0
bswap16 sp
bswap16 1
bswap16 [r0]
tst16 c0
tst16 q0
tst16 sp
tst16 symbol
sext8 c0
sext8 q0
sext8 [r0]
neg16 c0
neg16 q0
neg16 1
mul8 r4,c0
mul8 c0,r4
mul8 q0,c0
mul8 c0,q0
mul8 c0,1
mul8 1,c0
mul8 c0,[c1]
bswap16
bswap16 r0,r1
tst16
tst16 r0,r1
mul8
mul8 c0
mul8 c0,c1,c2
sext8
sext8 r0,r1
neg16
neg16 r0,r1
# CHECK-COUNT-27: error:
