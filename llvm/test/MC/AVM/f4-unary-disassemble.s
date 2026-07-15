# RUN: llvm-mc -triple=avm --disassemble < %S/Inputs/f4-unary.txt | FileCheck %s

# CHECK: lsl16.1 r0
# CHECK: lsl16.1 r7
# CHECK: lsr16.1 r0
# CHECK: lsr16.1 r7
# CHECK: asr16.1 r0
# CHECK: asr16.1 r7
# CHECK: not16 r0
# CHECK: not16 r7
# CHECK: tst8 r0
# CHECK: tst8 r7
# CHECK: inc16 r0
# CHECK: inc16 r7
# CHECK: dec16 r0
# CHECK: dec16 r7
