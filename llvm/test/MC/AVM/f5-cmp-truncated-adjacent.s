# RUN: llvm-mc -triple=avm --disassemble < %S/Inputs/f5-truncated.txt 2>&1 | FileCheck %s
# RUN: llvm-mc -triple=avm --disassemble < %S/Inputs/f5-adjacent.txt 2>&1 | FileCheck %s --check-prefix=ADJ

# CHECK: warning: invalid instruction encoding
# ADJ: cmp{{[ \t]+}}r0, r0
# ADJ: mov{{[ \t]+}}c0, c1
# ADJ: cmp{{[ \t]+}}r7, r3
