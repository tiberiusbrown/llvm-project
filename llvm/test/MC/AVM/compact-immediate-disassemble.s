# RUN: llvm-mc -triple=avm --disassemble < %S/Inputs/compact-immediate.txt | FileCheck %s

# CHECK: ldi8 r4, 0
# CHECK: ldi8 r5, 1
# CHECK: ldi8 r6, 127
# CHECK: ldi8 r7, 255
# CHECK: ldi16 r4, 0
# CHECK: ldi16 r5, 1
# CHECK: ldi16 r6, 4660
# CHECK: ldi16 r7, 65535
# CHECK: addi.s8 r4, -128
# CHECK: addi.s8 r5, -1
# CHECK: addi.s8 r6, 0
# CHECK: addi.s8 r7, 127
# CHECK: cmpi.s8 r4, -128
# CHECK: cmpi.s8 r5, -1
# CHECK: cmpi.s8 r6, 0
# CHECK: cmpi.s8 r7, 127
