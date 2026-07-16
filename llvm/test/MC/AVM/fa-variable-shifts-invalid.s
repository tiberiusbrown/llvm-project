# RUN: not llvm-mc -triple=avm %s 2>&1 | FileCheck %s
# CHECK-COUNT-36: error:
shl16v r4,c0
shl16v q0,c0
shl16v sp,c0
shl16v pc,c0
shl16v cc,c0
lsr16v r4,c0
asr16v q0,c0
shl16v c0,r4
shl16v c0,q0
shl16v c0,sp
shl16v c0,pc
shl16v c0,cc
lsr16v c0,r4
asr16v c0,q0
shl16v c0,1
shl16v 1,c0
shl16v c0,symbol
shl16v c0,[c1]
lsr16v c0,1
lsr16v [c0],c1
asr16v c0,symbol
asr16v c0,[c1]
shl16v
shl16v c0
shl16v c0,c1,c2
lsr16v
lsr16v c0
lsr16v c0,c1,c2
asr16v
asr16v c0
asr16v c0,c1,c2
lsl16v c0,c1
shl16 c0,c1
lsr16 c0,c1
asr16 c0,c1
shlv c0,c1
