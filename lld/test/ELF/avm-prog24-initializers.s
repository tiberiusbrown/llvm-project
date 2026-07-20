# RUN: split-file %s %t
# RUN: llvm-mc -triple=avm -filetype=obj %t/input.s -o %t/input.o
# RUN: ld.lld -T %t/layout.ld %t/input.o -o %t/output
# RUN: llvm-objdump -s -j .rodata -j .data %t/output | FileCheck %s
# RUN: llvm-mc -triple=avm -filetype=obj %t/overflow.s -o %t/overflow.o
# RUN: not ld.lld --entry=0 --image-base=0 --section-start=.data=0x100 \
# RUN:   --defsym=too_high=0x1000000 %t/overflow.o -o %t/bad 2>&1 \
# RUN:   | FileCheck %s --check-prefix=OVERFLOW

# CHECK: Contents of section .rodata:
# CHECK-NEXT: 123460 583412
# CHECK: Contents of section .data:
# CHECK-NEXT: 0100 583412
# OVERFLOW: relocation R_AVM_PROG24 out of range

#--- input.s
.section .text,"ax"
.globl _start
.globl target
_start:
target:
  nop

.section .rodata,"a"
  .progptr target

.section .data,"aw"
  .progptr target

#--- layout.ld
SECTIONS {
  .text 0x123458 : { *(.text) }
  .rodata 0x123460 : { *(.rodata) }
  .data 0x100 : { *(.data) }
}

#--- overflow.s
.data
  .progptr too_high
