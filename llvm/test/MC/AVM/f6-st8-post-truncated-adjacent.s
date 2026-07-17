# RUN: llvm-mc -triple=avm --disassemble < %S/Inputs/f6-st8-post-truncated.txt 2>&1 | FileCheck --check-prefix=TRUNC %s
# RUN: llvm-mc -triple=avm --disassemble < %S/Inputs/f6-st8-post-truncated-adjacent.txt | FileCheck --check-prefix=ADJ %s

# TRUNC: invalid instruction encoding
# ADJ: nop
# ADJ: st8 [r4+], r0
