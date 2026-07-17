# RUN: llvm-mc -triple=avm --disassemble < %S/Inputs/general-pointer-boundaries.txt | FileCheck %s

# CHECK: nop
# CHECK: ldi8{{[ \t]+}}r4, 18
# CHECK: ld8u{{[ \t]+}}r0, [r0]
# CHECK: jmpf{{[ \t]+}}0
# CHECK: st16{{[ \t]+}}[r7+], r7

# RUN: llvm-mc -triple=avm --disassemble < %S/Inputs/general-pointer-reserved-boundary.txt 2>&1 | FileCheck --check-prefix=RESERVED-BOUNDARY %s
# RESERVED-BOUNDARY: warning: invalid instruction encoding
# RESERVED-BOUNDARY: nop
