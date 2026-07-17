# RUN: not llvm-mc -triple=avm < %s 2>&1 | FileCheck %s

cmov.ule r0, r0
cmov.ugt r0, r0
cmov.c r0, r0
cmov.nc r0, r0
cmov.z r0, r0
cmov.nz r0, r0
cmov.lt r0, r0
cmov.ge r0, r0
cmov r0, r0
cmov.eq. r0, r0
cmoveq r0, r0
cmov.eq c0, r0
cmov.eq r0, c0
cmov.eq q0, r0
cmov.eq r0, q0
cmov.eq sp, r0
cmov.eq r0, pc
cmov.eq r0, 1
cmov.eq [r0], r1
cmov.eq
cmov.eq r0
cmov.eq r0, r1, r2
# CHECK-COUNT-20: error:
