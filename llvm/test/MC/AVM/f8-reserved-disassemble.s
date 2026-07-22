# RUN: llvm-mc -triple=avm --disassemble < %S/Inputs/f8-reserved.txt 2>&1 | FileCheck %s
# CHECK-COUNT-192: warning: invalid instruction encoding
# CHECK-NOT: cset.
