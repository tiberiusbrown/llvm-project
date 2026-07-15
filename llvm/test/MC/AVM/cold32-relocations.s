# RUN: llvm-mc -triple=avm -filetype=obj %s -o %t.o
# RUN: llvm-readobj --relocations %t.o | FileCheck %s
cmp32 q0,q1
ld32 q0,[r0]
st32 [r1],q2
# CHECK: Relocations [
# CHECK-NEXT: ]
