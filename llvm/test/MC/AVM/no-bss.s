# RUN: not llvm-mc -triple=avm -filetype=obj %s -o %t.o 2>&1 | FileCheck %s

.bss
.section .bss,"aw",@nobits
.section .zerodata,"aw",@nobits
.comm global, 4
.lcomm local, 4

# CHECK-COUNT-5: error: AVM does not support
