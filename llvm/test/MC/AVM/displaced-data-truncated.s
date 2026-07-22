# RUN: llvm-mc -triple=avm --disassemble < %S/Inputs/displaced-data-truncated-ed-1.txt 2>&1 | FileCheck %s
# RUN: llvm-mc -triple=avm --disassemble < %S/Inputs/displaced-data-truncated-ed-2.txt 2>&1 | FileCheck %s
# RUN: llvm-mc -triple=avm --disassemble < %S/Inputs/displaced-data-truncated-ee-1.txt 2>&1 | FileCheck %s
# RUN: llvm-mc -triple=avm --disassemble < %S/Inputs/displaced-data-truncated-ee-2.txt 2>&1 | FileCheck %s

# CHECK: warning: invalid instruction encoding
