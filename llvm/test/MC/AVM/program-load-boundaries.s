# RUN: llvm-mc -triple=avm --disassemble < %S/Inputs/program-load-boundaries.txt | FileCheck %s

# CHECK: mov{{[ \t]+}}c0, c0
# CHECK: ldi8{{[ \t]+}}c0, 18
# CHECK: ldp8u{{[ \t]+}}r0, [q0]
# CHECK: jmpf{{[ \t]+}}0
# CHECK: ldp32{{[ \t]+}}q0, [q1+]
