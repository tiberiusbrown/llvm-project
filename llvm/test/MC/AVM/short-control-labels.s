# RUN: llvm-mc -triple=avm -filetype=obj %s -o %t.o
# RUN: llvm-objdump -d --triple=avm %t.o | FileCheck %s

.text
breq forward
.byte 0
forward:
jmp8 backward
.byte 0
backward:
brne forward

# CHECK: d0 01
# CHECK: d4 01
# CHECK: d1 fb
