# RUN: llvm-mc -triple=avm-unknown-arduboyfx -show-encoding %s | FileCheck %s
# RUN: not llvm-mc -triple=avm-unknown-arduboyfx %S/Inputs/f3-ldsp8u-expression-invalid.s 2>&1 | FileCheck %S/Inputs/f3-ldsp8u-expression-invalid.s

ldsp8u c0, [sp+(1+2)]
ldsp8u c1, [sp+8-1]
ldsp8u c2, [sp+(4*3)]
ldsp8u c3, [sp+15]

# CHECK: encoding: [0xf3,0x4c]
# CHECK: encoding: [0xf3,0x5d]
# CHECK: encoding: [0xf3,0x72]
# CHECK: encoding: [0xf3,0x7f]
