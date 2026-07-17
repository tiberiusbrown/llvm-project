# RUN: llvm-mc -triple=avm --disassemble < %S/Inputs/f7-post-memory-valid.txt | FileCheck %s

# CHECK-COUNT-88: r
