# RUN: llvm-mc -triple=avm --disassemble < %S/Inputs/program-load-boundaries.txt | FileCheck %s

# CHECK: nop
# CHECK: ldi8{{[ \t]+}}r4, 18
# CHECK: ldp8u{{[ \t]+}}r0, [q0]
# CHECK: jmpf{{[ \t]+}}0
# CHECK: ldp32{{[ \t]+}}q0, [q1+]
