# RUN: llvm-mc -triple=avm --disassemble < %S/Inputs/f6-reserved.txt 2>&1 | FileCheck %s
# CHECK-COUNT-163: warning: invalid instruction encoding
