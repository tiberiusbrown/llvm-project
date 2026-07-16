# RUN: split-file %s %t
# RUN: llvm-mc -triple=avm -filetype=obj %t/start.s -o %t/start.o
# RUN: llvm-mc -triple=avm -filetype=obj %t/data.s -o %t/data.o
# RUN: llvm-mc -triple=avm -filetype=obj %t/overflow.s -o %t/overflow.o
# RUN: llvm-mc -triple=avm -filetype=obj %t/data-entry.s -o %t/data-entry.o
# RUN: ld.lld %t/start.o %t/data.o -o %t/a.out
# RUN: llvm-readobj --file-headers --sections --symbols %t/a.out | FileCheck %s --check-prefix=LAYOUT
# RUN: not ld.lld %t/overflow.o %t/start.o %t/data.o -o %t/overflow.out 2>&1 | FileCheck %s --check-prefix=OVERFLOW
# RUN: not ld.lld %t/data-entry.o -o %t/data-entry.out 2>&1 | FileCheck %s --check-prefix=DATA-ENTRY

# LAYOUT: Type: Executable (0x2)
# LAYOUT: Entry: 0x200
# LAYOUT: ProgramHeaderCount: 0
# LAYOUT: Name: .saved
# LAYOUT-NEXT: Type: SHT_PROGBITS
# LAYOUT: Address: 0x100
# LAYOUT: Size: 1
# LAYOUT: Name: .data
# LAYOUT-NEXT: Type: SHT_PROGBITS
# LAYOUT: Address: 0x101
# LAYOUT: Size: 1
# LAYOUT: Name: .text
# LAYOUT-NEXT: Type: SHT_PROGBITS
# LAYOUT: Address: 0x200
# LAYOUT: Name: _start
# LAYOUT: Value: 0x200

# OVERFLOW: AVM static storage exceeds the 1024-byte limit
# DATA-ENTRY: AVM entry symbol _start must be in an executable program-space section

#--- start.s
.text
.globl _start
_start:
  .short state
  .short runtime

#--- data.s
.section .saved.state,"aw",@progbits
.globl state
state:
  .byte 0x12

.section .data.runtime,"aw",@progbits
.globl runtime
runtime:
  .byte 0x34

#--- overflow.s
.section .data.too_large,"aw",@progbits
.zero 1025

#--- data-entry.s
.text
dummy:
  .byte 0

.data
.globl _start
_start:
  .byte 0
