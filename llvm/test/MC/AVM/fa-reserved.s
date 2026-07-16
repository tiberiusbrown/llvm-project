# RUN: llvm-mc -triple=avm --disassemble < %S/Inputs/fa-reserved.txt 2>&1 | FileCheck %s
# CHECK-COUNT-16: warning: invalid instruction encoding
# CHECK-NOT: shl16v
# CHECK-NOT: lsr16v
# CHECK-NOT: asr16v
# CHECK-NOT: lsl16i
# CHECK-NOT: lsr16i
# CHECK-NOT: asr16i
