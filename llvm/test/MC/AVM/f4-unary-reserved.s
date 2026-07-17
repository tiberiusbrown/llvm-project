# RUN: llvm-mc -triple=avm --disassemble < %S/Inputs/f4-unary-reserved.txt 2>&1 | FileCheck %s

# CHECK-COUNT-59: invalid instruction encoding
