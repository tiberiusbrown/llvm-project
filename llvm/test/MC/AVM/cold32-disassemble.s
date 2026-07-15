# RUN: cat %S/Inputs/cold32-valid.txt | llvm-mc -triple=avm --disassemble | FileCheck %s
# CHECK-COUNT-16: cmp32
# CHECK-COUNT-32: ld32
# CHECK-COUNT-32: st32
