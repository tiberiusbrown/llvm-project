# RUN: llvm-mc -triple=avm --disassemble < %S/Inputs/f1-reserved.txt 2>&1 | FileCheck %s

# CHECK-COUNT-99: warning: invalid instruction encoding
