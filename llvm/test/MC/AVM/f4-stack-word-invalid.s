# RUN: not llvm-mc -triple=avm-unknown-arduboyfx %s 2>&1 | FileCheck %s

ldsp16 q0, [sp+0]
ldsp16 sp, [sp+0]
ldsp16 pc, [sp+0]
ldsp16 cc, [sp+0]
ldsp16 c0, [sp-1]
ldsp16 c0, [sp+16]
ldsp16 c0, [sp+255]
ldsp16 c0, [sp+symbol]
ldsp16 c0, [sp]
ldsp16 c0, [r0]
ldsp16 [sp+0], c0
ldsp16
ldsp16 c0
ldsp16 c0, [sp+0], c1
stsp16 [sp+0], q0
stsp16 [sp+0], sp
stsp16 [sp+0], pc
stsp16 [sp+0], cc
stsp16 [sp-1], c0
stsp16 [sp+16], c0
stsp16 [sp+255], c0
stsp16 [sp+symbol], c0
stsp16 [sp], c0
stsp16 [r0], c0
stsp16 c0, [sp+0]
stsp16
stsp16 [sp+0]
stsp16 [sp+0], c0, c1

# CHECK-COUNT-28: error:
