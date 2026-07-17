# RUN: not llvm-mc -triple=avm -filetype=null %s 2>&1 | FileCheck %s
# CHECK-COUNT-20: error:
mul16 c0,r0
mul16 q0,r0
mul16 sp,r0
mul16 pc,r0
mul16 cc,r0
mul16 r0,c0
mul16 r0,q0
mul16 r0,sp
mul16 r0,pc
mul16 r0,cc
mul16 r0,1
mul16 1,r0
mul16 r0,symbol
mul16 symbol,r0
mul16 r0,[r1]
mul16 [r0],r1
mul16
mul16 r0
mul16 r0,r1,r2
muls16 r0,r1
mulu16 r0,r1
mul16u r0,r1
