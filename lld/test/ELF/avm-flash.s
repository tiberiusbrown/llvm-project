# RUN: split-file %s %t
# RUN: llvm-mc -triple=avm -filetype=obj %t/start.s -o %t/start.o
# RUN: llvm-mc -triple=avm -filetype=obj %t/exact.s -o %t/exact.o
# RUN: llvm-mc -triple=avm -filetype=obj %t/overflow.s -o %t/overflow.o
# RUN: llvm-mc -triple=avm -filetype=obj %t/part1.s -o %t/part1.o
# RUN: llvm-mc -triple=avm -filetype=obj %t/part2.s -o %t/part2.o
# RUN: llvm-mc -triple=avm -filetype=obj %t/aligned.s -o %t/aligned.o
# RUN: llvm-mc -triple=avm -filetype=obj %t/gcstart.s -o %t/gcstart.o
# RUN: llvm-mc -triple=avm -filetype=obj %t/unused.s -o %t/unused.o
# RUN: llvm-mc -triple=avm -filetype=obj %t/small.s -o %t/small.o
# RUN: llvm-mc -triple=avm -filetype=obj %t/data.s -o %t/data.o
# RUN: llvm-mc -triple=avm -filetype=obj %t/writable.s -o %t/writable.o
# RUN: llvm-mc -triple=avm -filetype=obj %t/align.s -o %t/align.o
# RUN: llvm-mc -triple=avm -filetype=obj %t/comdat1.s -o %t/comdat1.o
# RUN: llvm-mc -triple=avm -filetype=obj %t/comdat2.s -o %t/comdat2.o
# RUN: llvm-mc -triple=avm -filetype=obj %t/merge1.s -o %t/merge1.o
# RUN: llvm-mc -triple=avm -filetype=obj %t/merge2.s -o %t/merge2.o
# RUN: not llvm-mc -triple=avm -filetype=obj %t/nobits.s -o /dev/null 2>&1 \
# RUN:   | FileCheck %s --check-prefix=NOBITS

# Exact capacity links, and the window's first and last AS0 addresses are kept.
# RUN: ld.lld %t/start.o %t/exact.o -o %t/exact.elf
# RUN: llvm-readobj --sections --symbols %t/exact.elf | FileCheck %s --check-prefix=EXACT

# One byte over capacity is rejected with the requested capacities.
# RUN: not ld.lld %t/start.o %t/overflow.o -o %t/overflow.elf 2>&1 \
# RUN:   | FileCheck %s --check-prefix=OVERFLOW

# The total of several retained inputs is checked against the window.
# RUN: not ld.lld %t/start.o %t/part1.o %t/part2.o -o %t/multi.elf 2>&1 \
# RUN:   | FileCheck %s --check-prefix=MULTI

# Padding that alignment adds to the retained inputs counts toward capacity.
# RUN: not ld.lld %t/start.o %t/aligned.o -o %t/aligned.elf 2>&1 \
# RUN:   | FileCheck %s --check-prefix=ALIGNED

# An unreferenced oversized input is discarded before the capacity check.
# RUN: not ld.lld %t/start.o %t/unused.o %t/small.o -o %t/no-gc.elf 2>&1 \
# RUN:   | FileCheck %s --check-prefix=NO-GC
# RUN: ld.lld --gc-sections %t/gcstart.o %t/unused.o %t/small.o -o %t/gc.elf
# RUN: llvm-readobj --sections --symbols %t/gc.elf | FileCheck %s --check-prefix=GC

# Without flash inputs there is no .flash output and the program layout is the
# historical one.
# RUN: ld.lld %t/start.o -o %t/legacy.elf
# RUN: llvm-readobj --sections %t/legacy.elf | FileCheck %s --check-prefix=LEGACY

# A pointer in .data holds the AS0 flash VMA, and .text follows the flash bytes.
# RUN: ld.lld %t/start.o %t/data.o -o %t/data.elf
# RUN: llvm-readobj --sections %t/data.elf | FileCheck %s --check-prefix=DATA-LAYOUT
# RUN: llvm-objdump -s -j .data %t/data.elf | FileCheck %s --check-prefix=DATA-BYTES

# Custom layouts keep .flash at the required VMA and fail without it.
# RUN: ld.lld --image-base=0x100 --section-start=.flash=0xb00 \
# RUN:   --section-start=.text=0x600 \
# RUN:   %t/start.o %t/small.o -o %t/custom.elf
# RUN: llvm-readobj --sections %t/custom.elf | FileCheck %s --check-prefix=CUSTOM
# RUN: not ld.lld --image-base=0x100 --section-start=.text=0x600 %t/start.o \
# RUN:   %t/small.o \
# RUN:   -o %t/custom-bad.elf 2>&1 | FileCheck %s --check-prefix=CUSTOM-ERR

# Writable and over-aligned flash inputs are rejected.
# RUN: not ld.lld %t/start.o %t/writable.o -o %t/writable.elf 2>&1 \
# RUN:   | FileCheck %s --check-prefix=WRITABLE
# RUN: not ld.lld %t/start.o %t/align.o -o %t/align.elf 2>&1 \
# RUN:   | FileCheck %s --check-prefix=ALIGN

# COMDAT groups and mergeable strings keep their single retained copy.
# RUN: ld.lld %t/start.o %t/comdat1.o %t/comdat2.o -o %t/comdat.elf
# RUN: llvm-readobj --sections --symbols %t/comdat.elf | FileCheck %s --check-prefix=COMDAT
# RUN: ld.lld %t/start.o %t/merge1.o %t/merge2.o -o %t/merge.elf
# RUN: llvm-readobj --sections %t/merge.elf | FileCheck %s --check-prefix=MERGE

# EXACT:      Name: .flash
# EXACT-NEXT: Type: SHT_PROGBITS
# EXACT-NEXT: Flags [
# EXACT-NEXT:   SHF_ALLOC
# EXACT-NEXT:   SHF_AVM_DATASPACE
# EXACT-NEXT: ]
# EXACT:      Address: 0xB00
# EXACT:      Size: 62720
# EXACT:      Name: .text
# EXACT:      Address: 0xFA00
# EXACT:      Name: first
# EXACT-NEXT: Value: 0xB00
# EXACT:      Name: last
# EXACT-NEXT: Value: 0xFFFE

# OVERFLOW: ld.lld: error: AVM flash window overflow:
# OVERFLOW: 62721 bytes required, 62720 bytes available
# OVERFLOW: (AS0 addresses 0x0B00-0xFFFF)

# MULTI: error: AVM flash window overflow:
# MULTI: 65536 bytes required, 62720 bytes available

# ALIGNED: error: AVM flash window overflow:
# ALIGNED: 62736 bytes required, 62720 bytes available

# NO-GC: error: AVM flash window overflow:
# NO-GC: 65552 bytes required, 62720 bytes available

# GC:      Name: .flash
# GC-NEXT: Type: SHT_PROGBITS
# GC:      Address: 0xB00
# GC:      Size: 16
# GC:      Name: .text
# GC:      Address: 0x510
# GC-NOT:  unused

# LEGACY-NOT: Name: .flash
# LEGACY:     Name: .text
# LEGACY-NEXT: Type: SHT_PROGBITS
# LEGACY:      Address: 0x100

# DATA-LAYOUT:      Name: .flash
# DATA-LAYOUT:      Address: 0xB00
# DATA-LAYOUT:      Size: 4
# DATA-LAYOUT:      Name: .text
# DATA-LAYOUT:      Address: 0x504

# DATA-BYTES: Contents of section .data:
# DATA-BYTES-NEXT: 0100 000b

# CUSTOM-DAG: Name: .flash
# CUSTOM-DAG: Address: 0xB00
# CUSTOM-DAG: Name: .text
# CUSTOM-DAG: Address: 0x600
# CUSTOM-ERR: error: AVM .flash must begin at data address 0x0B00

# WRITABLE: error: AVM .flash must be allocated read-only data-space SHT_PROGBITS

# ALIGN: error: AVM .flash alignment 512 exceeds the 256-byte flash window alignment

# COMDAT:      Name: .flash
# COMDAT:      Size: 4
# COMDAT:      Name: shared
# COMDAT-NEXT: Value: 0xB00

# MERGE:      Name: .flash
# MERGE:      Size: 4

# NOBITS: AVM does not support allocated NOBITS or .bss storage

#--- start.s
.text
.globl _start
_start:
  .short 0

#--- exact.s
.section .flash.exact,"a",@progbits
.globl first
first:
  .zero 0xF4FE
.globl last
last:
  .short 0x5a5a

#--- overflow.s
.section .flash.overflow,"a",@progbits
  .zero 0xF501

#--- part1.s
.section .flash.part1,"a",@progbits
  .zero 0x8000

#--- part2.s
.section .flash.part2,"a",@progbits
  .zero 0x8000

#--- aligned.s
.section .flash.a,"a",@progbits
  .zero 0xF4F0
.section .flash.b,"a",@progbits
  .p2align 8
  .zero 0x10

#--- gcstart.s
.text
.globl _start
_start:
  .short small

#--- unused.s
.section .flash.unused,"a",@progbits
  .zero 0x10000

#--- small.s
.section .flash.small,"a",@progbits
.globl small
small:
  .zero 0x10

#--- data.s
.data
.globl pointer
pointer:
  .short table
.section .flash.table,"a",@progbits
.globl table
table:
  .byte 1, 2, 3, 4

#--- writable.s
.section .flash.writable,"aw",@progbits
  .zero 4

#--- align.s
.section .flash.wide,"a",@progbits
  .p2align 9
  .zero 4

#--- comdat1.s
.section .flash.shared,"aG",@progbits,shared,comdat
.globl shared
shared:
  .long 0x11223344

#--- comdat2.s
.section .flash.shared,"aG",@progbits,shared,comdat
.globl shared
shared:
  .long 0x11223344

#--- merge1.s
.section .flash.str,"aMS",@progbits,1
  .asciz "abc"

#--- merge2.s
.section .flash.str,"aMS",@progbits,1
  .asciz "abc"

#--- nobits.s
.section .flash.bss,"aw",@nobits
  .zero 4
