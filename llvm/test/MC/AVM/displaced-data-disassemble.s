# RUN: llvm-mc -triple=avm --disassemble < %S/Inputs/displaced-data-valid.txt | FileCheck %s

# CHECK: ld8u{{[ \t]+}}r0, [r0-32]
# CHECK: ld8u{{[ \t]+}}r3, [r5+0]
# CHECK: ld16{{[ \t]+}}r4, [r2-1]
# CHECK: ld16{{[ \t]+}}r7, [r7+223]
# CHECK: st8{{[ \t]+}}[r7-32], r0
# CHECK: st8{{[ \t]+}}[r5+0], r3
# CHECK: st16{{[ \t]+}}[r2-1], r4
# CHECK: st16{{[ \t]+}}[r0+223], r7
# CHECK: ld8u{{[ \t]+}}r1, [r2+1]
# CHECK: st8{{[ \t]+}}[r2+1], r1
