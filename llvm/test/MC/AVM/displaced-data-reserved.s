# RUN: llvm-mc -triple=avm --disassemble < %S/Inputs/displaced-data-reserved.txt 2>&1 | FileCheck %s

# CHECK-COUNT-2: warning: invalid instruction encoding
# CHECK-NOT: ld8u
# CHECK-NOT: st8
