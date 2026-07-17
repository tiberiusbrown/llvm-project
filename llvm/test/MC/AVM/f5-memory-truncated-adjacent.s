# RUN: llvm-mc -triple=avm --disassemble < %S/Inputs/f5-memory-truncated-adjacent.txt 2>&1 | FileCheck %s
# CHECK: warning: invalid instruction encoding
# CHECK: ld8u{{[[:space:]]+}}r0, [r4]
# CHECK: nop
