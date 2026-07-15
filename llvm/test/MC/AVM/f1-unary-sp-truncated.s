# RUN: llvm-mc -triple=avm --disassemble < %S/Inputs/f1-unary-sp-truncated.txt 2>&1 | FileCheck %s

# CHECK: warning: invalid instruction encoding
