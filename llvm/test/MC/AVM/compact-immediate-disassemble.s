# RUN: llvm-mc -triple=avm --disassemble < %S/Inputs/compact-immediate.txt | FileCheck %s

# CHECK: ldi8 c0, 0
# CHECK: ldi8 c1, 1
# CHECK: ldi8 c2, 127
# CHECK: ldi8 c3, 255
# CHECK: ldi16 c0, 0
# CHECK: ldi16 c1, 1
# CHECK: ldi16 c2, 4660
# CHECK: ldi16 c3, 65535
# CHECK: addi.s8 c0, -128
# CHECK: addi.s8 c1, -1
# CHECK: addi.s8 c2, 0
# CHECK: addi.s8 c3, 127
# CHECK: cmpi.s8 c0, -128
# CHECK: cmpi.s8 c1, -1
# CHECK: cmpi.s8 c2, 0
# CHECK: cmpi.s8 c3, 127
