# RUN: llvm-mc -triple=avm --disassemble < %S/Inputs/fa-adjacent.txt | FileCheck %s
# CHECK: shl16v c0, c0
# CHECK: lsr16v c3, c3
# CHECK: asr16v c1, c2
# CHECK: lsl16i c0, 0
# CHECK: lsl16i c0, 15
# CHECK: lsl16i c1, 0
# CHECK: lsr16i c0, 0
# CHECK: lsr16i c3, 15
# CHECK: asr16i c0, 0
# CHECK: asr16i c3, 15
# CHECK-NOT: <unknown>
