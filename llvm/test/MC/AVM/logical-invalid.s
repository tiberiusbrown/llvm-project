# RUN: not llvm-mc -triple=avm-unknown-arduboyfx %s 2>&1 | FileCheck %s

and r0, r1
or c1, r0
xor c2, r3
bic r7, r0

# CHECK: error: logical operation requires destination c0/A, or compact destination c1-c3 and compact source c0-c3
# CHECK: error: logical operation requires destination c0/A, or compact destination c1-c3 and compact source c0-c3
# CHECK: error: logical operation requires destination c0/A, or compact destination c1-c3 and compact source c0-c3
# CHECK: error: logical operation requires destination c0/A, or compact destination c1-c3 and compact source c0-c3
