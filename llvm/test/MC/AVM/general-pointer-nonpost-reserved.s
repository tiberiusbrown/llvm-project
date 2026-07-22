# RUN: llvm-mc -triple=avm --disassemble < %S/Inputs/general-pointer-nonpost-reserved.txt 2>&1 | FileCheck %s

# All 256 non-postincrement F0 6c/6d operand bytes are reserved.
# CHECK-COUNT-256: warning: invalid instruction encoding

