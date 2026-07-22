# RUN: not llvm-mc -triple=avm -show-encoding %s 2>&1 | FileCheck %s

ld8u r0, [r1-33]
ld8u r0, [r1+224]
st16 [r2-33], r3
st16 [r2+224], r3
ld8u r0, [r1+symbol]

# CHECK-COUNT-4: error: displacement is out of range; expected -32 through 223
# CHECK: error: displacement expression must be fully resolvable
