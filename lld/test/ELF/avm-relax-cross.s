# RUN: split-file %s %t
# RUN: llvm-mc -triple=avm -filetype=obj %t/a.s -o %t/a.o
# RUN: llvm-mc -triple=avm -filetype=obj %t/b.s -o %t/b.o
# RUN: ld.lld -T %t/layout.ld %t/a.o %t/b.o -o %t/cross.out
# RUN: llvm-objdump -s -j .text.a -j .text.b -j .text.local -j .far %t/cross.out | FileCheck %s

# A local target in another input section, global targets in another object,
# a positive and negative addend, and a 16-bit target in another output section.
# CHECK: Contents of section .text.a:
# CHECK-NEXT: 10000 d46ed47d d579e0f9 7f
# CHECK: Contents of section .text.local:
# CHECK-NEXT: 10070 00
# CHECK: Contents of section .text.b:
# CHECK-NEXT: 10080 e17dff00
# CHECK: Contents of section .far:
# CHECK-NEXT: 18002 00

#--- a.s
.section .text.a
.globl source
source:
  jmp local_target
  jmp global_target+1
  call global_target-1
  jmp long_target

.section .text.local
local_target:
  .byte 0

#--- b.s
.section .text.b
.globl global_target
global_target:
  call source
  .byte 0

.section .far,"ax"
.globl long_target
long_target:
  .byte 0

#--- layout.ld
SECTIONS {
  .text.a 0x10000 : { *(.text.a) }
  .text.local 0x10070 : { *(.text.local) }
  .text.b 0x10080 : { *(.text.b) }
  .far 0x18002 : { *(.far) }
}
