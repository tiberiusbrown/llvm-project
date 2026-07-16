# RUN: split-file %s %t
# RUN: llvm-mc -triple=avm -filetype=obj %t/source.s -o %t/source.o
# RUN: llvm-mc -triple=avm -filetype=obj %t/target.s -o %t/target.o
# RUN: ld.lld -T %t/layout.ld %t/source.o %t/target.o -o %t/alignment.out
# RUN: llvm-readobj --sections %t/alignment.out | FileCheck %s --check-prefix=SECTIONS
# RUN: llvm-objdump -s -j .text -j .target %t/alignment.out | FileCheck %s --check-prefix=BYTES

# The original 130-byte source section places .target at 0x10100. Relaxation
# first picks JMP16; the one-byte shrink moves .target to 0x10080, making the
# two-byte form valid on the next iteration. The final 128-byte source keeps
# the aligned target at 0x10080, so the layout is stable.
# SECTIONS: Name: .text
# SECTIONS: Address: 0x10000
# SECTIONS: Size: 128
# SECTIONS: Name: .target
# SECTIONS: Address: 0x10080
# SECTIONS: Size: 1
# BYTES: Contents of section .text:
# BYTES-NEXT: 10000 d47e
# BYTES: Contents of section .target:
# BYTES-NEXT: 10080 00

#--- source.s
.text
.globl _start
_start:
jmp target
.zero 125

#--- target.s
.section .text.target,"ax"
.p2align 7
.globl target
target:
.byte 0

#--- layout.ld
SECTIONS {
  .text 0x10000 : { *(.text) }
  .target : { *(.text.target) }
}
