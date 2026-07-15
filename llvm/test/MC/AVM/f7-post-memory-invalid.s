# RUN: not llvm-mc -triple=avm -filetype=null %s 2>&1 | FileCheck %s

ld8u r4, [c0+]
ld8u r5, [c1+]
ld8u r6, [c2+]
ld8u r7, [c3+]
ld16 r4, [c0+]
ld16 r5, [c1+]
ld16 r6, [c2+]
ld16 r7, [c3+]

# CHECK-COUNT-8: postincrement destination must not overlap address register

