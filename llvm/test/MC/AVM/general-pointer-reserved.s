# RUN: llvm-mc -triple=avm --disassemble < %S/Inputs/general-pointer-reserved.txt 2>&1 | FileCheck %s

# All F0 6e-ff secondaries are reserved and consume no operand byte. The
# standalone EC byte in the exhaustive stream is now a valid two-byte form;
# ED and EE also begin valid three-byte forms.
# CHECK-COUNT-130: warning: invalid instruction encoding
