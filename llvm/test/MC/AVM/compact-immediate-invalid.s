# RUN: not llvm-mc -triple=avm -show-encoding %s 2>&1 | FileCheck %s

ldi8 r4, 1
ldi8 q0, 1
ldi8 sp, 1
ldi8 c0
ldi8 c0, 1, 2
ldi8 c0, symbol
ldi8 c0, -1
ldi8 c0, 256
ldi16 c0, -1
ldi16 c0, 65536
addi.s8 c0, -129
addi.s8 c0, 128
addi.s8 c0, 255
cmpi.s8 c0, -129
cmpi.s8 c0, 128
cmpi.s8 c0, 255

# CHECK: error: expected compact register r4-r7
# CHECK: error: expected compact register r4-r7
# CHECK: error: expected comma
# CHECK: error: unexpected token after AVM instruction
# CHECK: error: immediate expression must be fully resolvable
# CHECK-COUNT-10: error: immediate is out of range
