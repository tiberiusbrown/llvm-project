# RUN: split-file %s %t
# RUN: llvm-mc -triple=avm -filetype=obj %t/short-cascade.s -o %t/short-cascade.o
# RUN: ld.lld -Ttext=0x10000 %t/short-cascade.o -o %t/short-cascade.out
# RUN: llvm-readobj --sections %t/short-cascade.out | FileCheck %s --check-prefix=SHORT-SIZE
# RUN: llvm-objdump -s -j .text %t/short-cascade.out | FileCheck %s --check-prefix=SHORT-BYTES
# RUN: llvm-mc -triple=avm -filetype=obj %t/cond-staged.s -o %t/cond-staged.o
# RUN: ld.lld -Ttext=0x10000 %t/cond-staged.o -o %t/cond-staged.out
# RUN: llvm-readobj --sections %t/cond-staged.out | FileCheck %s --check-prefix=COND-SIZE
# RUN: llvm-objdump -s -j .text %t/cond-staged.out | FileCheck %s --check-prefix=COND-BYTES
# RUN: llvm-mc -triple=avm -filetype=obj %t/chain.s -o %t/chain.o
# RUN: ld.lld -Ttext=0x10000 %t/chain.o -o %t/chain.out
# RUN: llvm-readobj --sections %t/chain.out | FileCheck %s --check-prefix=CHAIN-SIZE
# RUN: llvm-objdump -s -j .text %t/chain.out | FileCheck %s --check-prefix=CHAIN-BYTES

# A forward jump is initially one byte beyond the direct range after accounting
# for its own shrink. A later direct jump removes two more bytes, making it fit.
# SHORT-SIZE: Name: .text
# SHORT-SIZE: Size: 129
# SHORT-BYTES: 10000 d47ed47c

# This conditional is initially 5 bytes; the following direct jump makes its
# two-byte form fit in the next layout iteration.
# COND-SIZE: Name: .text
# COND-SIZE: Size: 130
# COND-BYTES: 10000 d07fd47d

# The first site depends on both later sites. The final fixed point has three
# direct jumps and no oversized displacement.
# CHAIN-SIZE: Name: .text
# CHAIN-SIZE: Size: 130
# CHAIN-BYTES: 10000 d47fd47d d47b

#--- short-cascade.s
.text
jmp target
jmp target
.zero 124
target:
.byte 0

#--- cond-staged.s
.text
br.eq target
jmp target
.zero 125
target:
.byte 0

#--- chain.s
.text
jmp target
jmp target
jmp target
.zero 123
target:
.byte 0
