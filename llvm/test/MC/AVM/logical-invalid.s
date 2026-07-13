# RUN: not llvm-mc -triple=avm-unknown-arduboyfx %s 2>&1 | FileCheck %s

and r0, r1
or c1, r0
xor c2, r3
bic r7, r0
mov b0, b1
sub.nf c1, r0
cmp16 c1, c1
cmp8 c2, c2

# CHECK: error: logical operation requires destination c0/A, or compact destination c1-c3 and compact source c0-c3
# CHECK: error: logical operation requires destination c0/A, or compact destination c1-c3 and compact source c0-c3
# CHECK: error: logical operation requires destination c0/A, or compact destination c1-c3 and compact source c0-c3
# CHECK: error: logical operation requires destination c0/A, or compact destination c1-c3 and compact source c0-c3
# CHECK: error: MOV requires 16-bit registers
# CHECK: error: SUB.NF requires compact operands or destination A and source r0-r3
# CHECK: error: compact self-compare has no encoding
# CHECK: error: compact self-compare has no encoding
