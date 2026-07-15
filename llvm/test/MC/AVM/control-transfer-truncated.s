# RUN: llvm-mc -triple=avm -filetype=obj %s -o %t.o
# RUN: llvm-objdump -D --triple=avm %t.o | FileCheck %s

.section .e0-one
.byte 0xe0
.section .e0-two
.byte 0xe0, 0x00
.section .e1-one
.byte 0xe1
.section .e1-two
.byte 0xe1, 0x00
.section .e2-one
.byte 0xe2
.section .e2-two
.byte 0xe2, 0x00
.section .e2-three
.byte 0xe2, 0x00, 0x00
.section .e3-one
.byte 0xe3
.section .e3-two
.byte 0xe3, 0x00
.section .e3-three
.byte 0xe3, 0x00, 0x00

# CHECK-COUNT-10: <unknown>
