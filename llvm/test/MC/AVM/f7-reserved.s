# RUN: llvm-mc -triple=avm --disassemble < %S/Inputs/f7-reserved.txt 2>&1 | FileCheck %s
# CHECK-COUNT-103: invalid instruction encoding
