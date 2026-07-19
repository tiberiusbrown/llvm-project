# RUN: llvm-mc -triple=avm -filetype=obj -o %t %s
# RUN: llvm-objdump -s %t | FileCheck %s

.text
.byte 0x12
.p2align 3
.byte 0x34

# CHECK: 0000 12000000 00000000 34
