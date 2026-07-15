# RUN: llvm-mc -triple=avm --disassemble < %S/Inputs/f5-memory-reserved.txt 2>&1 | FileCheck %s
# CHECK-COUNT-160: warning: invalid instruction encoding
