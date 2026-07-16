# RUN: llvm-mc -triple=avm -filetype=obj %s -o %t.o
# RUN: llvm-readobj --sections %t.o | FileCheck %s

.section .text
  nop
.section .rodata
  .byte 1
.section .data
  .byte 2
.section .bss
  .zero 3

# CHECK: Name: .text
# CHECK: Flags [ (0x10000006)
# CHECK: Name: .rodata
# CHECK: Flags [ (0x10000002)
# CHECK: Name: .data
# CHECK: Flags [ (0x20000003)
# CHECK: Name: .bss
# CHECK: Flags [ (0x20000003)
