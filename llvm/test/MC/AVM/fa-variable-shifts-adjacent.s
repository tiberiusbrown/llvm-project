# RUN: llvm-mc -triple=avm --disassemble < %S/Inputs/fa-adjacent.txt | FileCheck %s
# CHECK: shl16v c0, c0
# CHECK: lsr16v c3, c3
# CHECK: asr16v c1, c2
# CHECK-NOT: <unknown>
