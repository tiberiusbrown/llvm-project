# RUN: llvm-mc -triple=avm-unknown-arduboyfx -filetype=obj %s -o %t.o
# RUN: llvm-readobj --relocations %t.o | FileCheck %s --check-prefix=RELOC
# RUN: llvm-objdump -dr %t.o | FileCheck %s --check-prefix=DIS

.section .text,"ax",@progbits
.globl _start
.globl global_target
.p2align 1
_start:
  jmp16 local_target
  call16 global_target + 2
  jmpf far_target
  callf global_target
  jmp16 rodata_target
  jmp16 0x1234
  callf 0x2468

local_target:
global_target:
  nop
.p2align 1
far_target:
  nop

.section .rodata,"a",@progbits
rodata_target:
  .byte 0

# RELOC: R_AVM_BANK16
# RELOC: R_AVM_BANK16
# RELOC: R_AVM_FAR24
# RELOC: R_AVM_FAR24
# RELOC: R_AVM_BANK16
# RELOC-NOT: R_AVM_{{.*}} 0x1234
# DIS: R_AVM_BANK16
# DIS: R_AVM_BANK16
# DIS: R_AVM_FAR24
# DIS: R_AVM_FAR24
# DIS: R_AVM_BANK16
