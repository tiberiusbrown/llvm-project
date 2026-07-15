# RUN: not llvm-mc -triple=avm-unknown-arduboyfx %s 2>&1 | FileCheck %s

ldsp8u q0, [sp+0]
ldsp8u sp, [sp+0]
ldsp8u pc, [sp+0]
ldsp8u cc, [sp+0]
ldsp8u c0, [sp-1]
ldsp8u c0, [sp+16]
ldsp8u c0, [sp+255]
ldsp8u c0, [sp+symbol]
ldsp8u c0, [sp]
ldsp8u c0, [r0]
ldsp8u c0, [r0+]
ldsp8u c0, 0
ldsp8u [sp+0], c0
ldsp8u
ldsp8u c0
ldsp8u c0, [sp+0], c1

# CHECK-COUNT-16: error:
