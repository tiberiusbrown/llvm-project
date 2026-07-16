# RUN: llvm-mc -triple=avm --disassemble < %S/Inputs/general-pointer-reserved.txt 2>&1 | FileCheck %s

# All 146 F0 6e-ff secondaries are reserved and consume no operand byte.
# CHECK-COUNT-136: warning: invalid instruction encoding
