# RUN: llvm-mc -triple=avm --disassemble < %S/Inputs/ec-divrem-reserved.txt 2>&1 | FileCheck %s
# CHECK-COUNT-2: warning: invalid instruction encoding
# CHECK-NOT: div16
