# RUN: not llvm-mc -triple=avm-unknown-arduboyfx -filetype=null %s 2>&1 | FileCheck %s

mul8 r3, r4
shl16v r1, r4
lsr16v r1, r4
lsl16i r1, 8

# CHECK: error: expected compact register r4-r7
# CHECK: error: expected compact register r4-r7
# CHECK: error: expected compact register r4-r7
# CHECK: error: expected compact register r4-r7
