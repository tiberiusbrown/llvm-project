# RUN: llvm-mc -triple=avm --disassemble < %S/Inputs/f2-reserved.txt 2>&1 | FileCheck %s

# CHECK-COUNT-149: invalid instruction encoding
