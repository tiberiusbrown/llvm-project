# RUN: not llvm-mc -triple=avm < %s 2>&1 | FileCheck %s

cset.eq c0
cset.eq q0
cset.eq sp
cset.eq pc
cset.eq cc
cset.ne 1
cset.ult [r0]
cset.uge symbol
cset.slt 0
cset.sge [sp+0]
cset.eq
cset.eq r0,r1
cset.ne
cset.ne r0,r1
cset.ult
cset.ult r0,r1
cset.uge
cset.uge r0,r1
cset.slt
cset.slt r0,r1
cset.sge
cset.sge r0,r1
cset.ule r0
cset.ugt r0
cset.c r0
cset.nc r0
cset.z r0
cset.nz r0
cset r0
cset. r0
cset.eq. r0
cseteq r0

# CHECK-COUNT-32: error:
