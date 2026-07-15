# RUN: llvm-mc -triple=avm --disassemble < %S/Inputs/general-pointer-truncated-f0.txt 2>&1 | FileCheck %s
# RUN: llvm-mc -triple=avm --disassemble < %S/Inputs/general-pointer-truncated-6c.txt 2>&1 | FileCheck %s
# RUN: llvm-mc -triple=avm --disassemble < %S/Inputs/general-pointer-truncated-6d.txt 2>&1 | FileCheck %s

# CHECK-COUNT-1: warning: invalid instruction encoding
