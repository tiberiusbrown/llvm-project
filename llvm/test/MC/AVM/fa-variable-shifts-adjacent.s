# RUN: llvm-mc -triple=avm --disassemble < %S/Inputs/fa-adjacent.txt | FileCheck %s
# CHECK: shl16v r4, r4
# CHECK: lsr16v r7, r7
# CHECK: asr16v r5, r6
# CHECK: lsl16i r4, 0
# CHECK: lsl16i r4, 15
# CHECK: lsl16i r5, 0
# CHECK: lsr16i r4, 0
# CHECK: lsr16i r7, 15
# CHECK: asr16i r4, 0
# CHECK: asr16i r7, 15
# CHECK-NOT: <unknown>
