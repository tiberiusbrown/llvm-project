# RUN: llvm-mc -triple=avm --disassemble < %S/Inputs/short-control-truncated-d0.txt 2>&1 | FileCheck %s
# RUN: llvm-mc -triple=avm --disassemble < %S/Inputs/short-control-truncated-d1.txt 2>&1 | FileCheck %s
# RUN: llvm-mc -triple=avm --disassemble < %S/Inputs/short-control-truncated-d2.txt 2>&1 | FileCheck %s
# RUN: llvm-mc -triple=avm --disassemble < %S/Inputs/short-control-truncated-d3.txt 2>&1 | FileCheck %s
# RUN: llvm-mc -triple=avm --disassemble < %S/Inputs/short-control-truncated-d4.txt 2>&1 | FileCheck %s
# RUN: llvm-mc -triple=avm --disassemble < %S/Inputs/short-control-truncated-d5.txt 2>&1 | FileCheck %s
# RUN: llvm-mc -triple=avm --disassemble < %S/Inputs/short-control-truncated-d6.txt 2>&1 | FileCheck %s
# RUN: llvm-mc -triple=avm --disassemble < %S/Inputs/short-control-truncated-d7.txt 2>&1 | FileCheck %s

# CHECK: warning: invalid instruction encoding
