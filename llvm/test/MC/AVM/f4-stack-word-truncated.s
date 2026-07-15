# RUN: llvm-mc -triple=avm-unknown-arduboyfx --disassemble < %S/Inputs/f4-truncated.txt 2>&1 | FileCheck %s

# CHECK: invalid instruction encoding
