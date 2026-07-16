# RUN: llvm-mc -triple=avm-unknown-arduboyfx -filetype=obj %s -o %t.o
# RUN: llvm-readobj --relocations --section-data %t.o | FileCheck %s

.text
.globl data
.globl program
ldi16 c0, data+4
ldi16 r0, %lo16(program+5)
ldi8 c1, %hi8(program-6)
ldi8 r1, %hi8(program)
.short data-2
.word %lo16(program+3)
.byte %hi8(program-2)
.progptr program+7
.progptr %prog24(program-8)
data:
program:

# CHECK: R_AVM_DATA16 data 0x4
# CHECK: R_AVM_PROG_LO16 program 0x5
# CHECK: R_AVM_PROG_HI8 program 0xFFFFFFFA
# CHECK: R_AVM_PROG_HI8 program 0x0
# CHECK: R_AVM_DATA16 data 0xFFFFFFFE
# CHECK: R_AVM_PROG_LO16 program 0x3
# CHECK: R_AVM_PROG_HI8 program 0xFFFFFFFE
# CHECK: R_AVM_PROG24 program 0x7
# CHECK: R_AVM_PROG24 program 0xFFFFFFF8
