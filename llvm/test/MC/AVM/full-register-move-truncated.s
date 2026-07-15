# RUN: llvm-mc -triple=avm --disassemble < %S/Inputs/full-register-move-truncated.txt 2>&1 | FileCheck %s

# CHECK: warning: invalid instruction encoding
