# RUN: not llvm-mc -triple=avm-unknown-arduboyfx %s 2>&1 | FileCheck %s

lsl16 b4
zext8 b0
sext8 b7
ldi8 b4, 1
tst16 b0
tst8 b7
mov8z r0, r0
mov8s b0, b0

# CHECK: error: LSL16 requires a 16-bit register
# CHECK: error: extension alias requires a 16-bit register
# CHECK: error: extension alias requires a 16-bit register
# CHECK: error: instruction requires a 16-bit register
# CHECK: error: test requires a 16-bit register
# CHECK: error: test requires a 16-bit register
# CHECK: error: MOV8Z/MOV8S require 'rD, bS' operands
# CHECK: error: MOV8Z/MOV8S require 'rD, bS' operands
