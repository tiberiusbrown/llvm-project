# RUN: llvm-mc -triple=avm --disassemble < %S/Inputs/f6-st8-post-truncated.txt 2>&1 | FileCheck --check-prefix=TRUNC %s
# RUN: llvm-mc -triple=avm --disassemble < %S/Inputs/f6-st8-post-truncated-adjacent.txt | FileCheck --check-prefix=ADJ %s

# TRUNC: invalid instruction encoding
# ADJ: mov c0, c0
# ADJ: st8 [c0+], r0
