# RUN: llvm-mc -triple=avm --disassemble < %S/Inputs/ff-floating-reserved.txt 2>&1 | FileCheck %s

# CHECK-COUNT-226: invalid instruction encoding
