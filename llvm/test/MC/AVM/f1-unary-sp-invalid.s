# RUN: not llvm-mc -triple=avm -filetype=null %s 2>&1 | FileCheck %s

zext8 c0
zext8 q0
zext8 sp
zext8 pc
zext8 cc
swap8 c0
swap8 q0
swap8 sp
getsp c0
getsp q0
getsp sp
setsp c0
setsp q0
setsp sp
zext8 1
zext8 symbol
zext8 [r0]
swap8 1
getsp [r0]
setsp symbol
zext8
zext8 r0,r1
swap8
swap8 r0,r1
getsp
getsp r0,r1
setsp
setsp r0,r1

# CHECK-COUNT-24: error:
