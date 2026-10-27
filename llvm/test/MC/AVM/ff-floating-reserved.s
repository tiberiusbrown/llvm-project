# RUN: llvm-mc -triple=avm --disassemble < %S/Inputs/ff-floating-reserved.txt 2>&1 | FileCheck %s

# CHECK-COUNT-233: invalid instruction encoding
