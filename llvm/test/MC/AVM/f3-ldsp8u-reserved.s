# RUN: llvm-mc -triple=avm-unknown-arduboyfx --disassemble < %S/Inputs/f3-ldsp8u-reserved.txt 2>&1 | FileCheck %s
# CHECK-COUNT-117: warning: invalid instruction encoding
