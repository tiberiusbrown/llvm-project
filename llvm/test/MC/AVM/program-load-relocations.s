# RUN: llvm-mc -triple=avm-unknown-arduboyfx -filetype=obj %s -o %t.o
# RUN: llvm-readobj --relocations %t.o | FileCheck %s

.text
ldp8u r0, [q0]
ldp16 r7, [q2+]
ldp24 q0, [q1]
ldp32 q3, [q2+]

# CHECK: Relocations [
# CHECK-NEXT: ]
