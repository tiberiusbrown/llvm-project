# RUN: split-file %s %t
# RUN: llvm-mc -triple=avm -filetype=obj %t/layout.s -o %t/layout.o
# RUN: ld.lld %t/layout.o -o %t/layout.out
# RUN: llvm-readobj --file-headers --sections %t/layout.out | FileCheck %s --check-prefix=LAYOUT
# RUN: llvm-mc -triple=avm -filetype=obj %t/empty.s -o %t/empty.o
# RUN: llvm-mc -triple=avm -filetype=obj %t/one.s -o %t/one.o
# RUN: llvm-mc -triple=avm -filetype=obj %t/size255.s -o %t/size255.o
# RUN: llvm-mc -triple=avm -filetype=obj %t/size256.s -o %t/size256.o
# RUN: llvm-mc -triple=avm -filetype=obj %t/size257.s -o %t/size257.o
# RUN: llvm-mc -triple=avm -filetype=obj %t/size1024.s -o %t/size1024.o
# RUN: llvm-mc -triple=avm -filetype=obj %t/size1025.s -o %t/size1025.o
# RUN: ld.lld %t/empty.o -o %t/empty.out
# RUN: llvm-readobj --sections %t/empty.out | FileCheck %s --check-prefix=EMPTY
# RUN: ld.lld %t/one.o -o %t/one.out
# RUN: llvm-readobj --sections %t/one.out | FileCheck %s --check-prefix=ONE
# RUN: ld.lld %t/size255.o -o %t/size255.out
# RUN: llvm-readobj --sections %t/size255.out | FileCheck %s --check-prefix=SIZE255
# RUN: ld.lld %t/size256.o -o %t/size256.out
# RUN: llvm-readobj --sections %t/size256.out | FileCheck %s --check-prefix=SIZE256
# RUN: ld.lld %t/size257.o -o %t/size257.out
# RUN: llvm-readobj --sections %t/size257.out | FileCheck %s --check-prefix=SIZE257
# RUN: ld.lld %t/size1024.o -o %t/size1024.out
# RUN: llvm-readobj --sections %t/size1024.out | FileCheck %s --check-prefix=SIZE1024
# RUN: not ld.lld %t/size1025.o -o %t/size1025.out 2>&1 | FileCheck %s --check-prefix=SIZE1025
# RUN: llvm-mc -triple=avm -filetype=obj %t/relax-entry.s -o %t/relax-entry.o
# RUN: ld.lld %t/relax-entry.o -o %t/relax-entry.out
# RUN: llvm-readobj --file-headers --symbols %t/relax-entry.out | FileCheck %s --check-prefix=RELAX
# RUN: llvm-mc -triple=avm -filetype=obj %t/missing.s -o %t/missing.o
# RUN: not ld.lld %t/missing.o -o %t/missing.out 2>&1 | FileCheck %s --check-prefix=MISSING
# RUN: llvm-mc -triple=avm -filetype=obj %t/rodata-entry.s -o %t/rodata-entry.o
# RUN: not ld.lld %t/rodata-entry.o -o %t/rodata-entry.out 2>&1 | FileCheck %s --check-prefix=BADENTRY
# RUN: llvm-mc -triple=avm -filetype=obj %t/gc-start.s -o %t/gc-start.o
# RUN: llvm-mc -triple=avm -filetype=obj %t/gc-live.s -o %t/gc-live.o
# RUN: llvm-mc -triple=avm -filetype=obj %t/gc-dead.s -o %t/gc-dead.o
# RUN: ld.lld --gc-sections %t/gc-start.o %t/gc-live.o %t/gc-dead.o -o %t/gc.out
# RUN: llvm-readobj --sections --symbols %t/gc.out | FileCheck %s --check-prefix=GC
# RUN: llvm-mc -triple=avm -filetype=obj %t/multi-a.s -o %t/multi-a.o
# RUN: llvm-mc -triple=avm -filetype=obj %t/multi-b.s -o %t/multi-b.o
# RUN: llvm-mc -triple=avm -filetype=obj %t/multi-c.s -o %t/multi-c.o
# RUN: ld.lld %t/multi-a.o %t/multi-b.o %t/multi-c.o -o %t/multi.out
# RUN: llvm-readobj --file-headers --sections --symbols %t/multi.out | FileCheck %s --check-prefix=MULTI
# RUN: llvm-objdump -s -j .text %t/multi.out | FileCheck %s --check-prefix=MULTIBYTES
# RUN: llvm-mc -triple=avm -filetype=obj %t/comdat-start.s -o %t/comdat-start.o
# RUN: llvm-mc -triple=avm -filetype=obj %t/comdat-a.s -o %t/comdat-a.o
# RUN: llvm-mc -triple=avm -filetype=obj %t/comdat-b.s -o %t/comdat-b.o
# RUN: ld.lld %t/comdat-start.o %t/comdat-a.o %t/comdat-b.o -o %t/comdat.out
# RUN: llvm-readobj --sections --symbols %t/comdat.out | FileCheck %s --check-prefix=COMDAT
# RUN: llvm-mc -triple=avm -filetype=obj %t/script.s -o %t/script.o
# RUN: ld.lld -T %t/good.ld %t/script.o -o %t/good.out
# RUN: llvm-readobj --file-headers --sections %t/good.out | FileCheck %s --check-prefix=GOOD
# RUN: not ld.lld -T %t/bad-saved.ld %t/script.o -o %t/bad-saved.out 2>&1 | FileCheck %s --check-prefix=BADSAVED
# RUN: not ld.lld -T %t/bad-data.ld %t/script.o -o %t/bad-data.out 2>&1 | FileCheck %s --check-prefix=BADDATA
# RUN: not ld.lld -T %t/bad-entry.ld %t/script.o -o %t/bad-entry.out 2>&1 | FileCheck %s --check-prefix=BADENTRY
# RUN: llvm-mc -triple=avm -filetype=obj %t/range.s -o %t/range.o
# RUN: llvm-mc -triple=avm -filetype=obj %t/high.s -o %t/high.o
# RUN: ld.lld -T %t/high.ld %t/high.o -o %t/high.out
# RUN: llvm-readobj --file-headers %t/high.out | FileCheck %s --check-prefix=HIGH
# RUN: ld.lld -T %t/cross.ld %t/range.o -o %t/cross.out
# RUN: llvm-readobj --sections %t/cross.out | FileCheck %s --check-prefix=CROSS
# RUN: not ld.lld -T %t/reserved.ld %t/range.o -o %t/reserved.out 2>&1 | FileCheck %s --check-prefix=RESERVED
# RUN: not ld.lld -T %t/overflow.ld %t/range.o -o %t/overflow.out 2>&1 | FileCheck %s --check-prefix=OVERFLOW

# LAYOUT: Format: elf32-avm
# LAYOUT: Arch: avm
# LAYOUT: Type: Executable (0x2)
# LAYOUT: Entry: 0x200
# LAYOUT: ProgramHeaderCount: 0
# LAYOUT: Name: .saved
# LAYOUT-NEXT: Type: SHT_PROGBITS
# LAYOUT: Address: 0x100
# LAYOUT: Name: .data
# LAYOUT-NEXT: Type: SHT_PROGBITS
# LAYOUT: Address: 0x101
# LAYOUT: Name: .text
# LAYOUT-NEXT: Type: SHT_PROGBITS
# LAYOUT: Flags [ (0x10000006)
# LAYOUT: Address: 0x200
# LAYOUT: Name: .rodata
# LAYOUT-NEXT: Type: SHT_PROGBITS
# LAYOUT: Flags [ (0x10000002)
# LAYOUT: Name: .init_array
# LAYOUT-NEXT: Type: SHT_INIT_ARRAY
# LAYOUT: EntrySize: 3
# LAYOUT: Name: .fini_array
# LAYOUT-NEXT: Type: SHT_FINI_ARRAY
# LAYOUT: EntrySize: 3
# EMPTY: Name: .text
# EMPTY: Address: 0x100
# ONE: Name: .data
# ONE: Size: 1
# ONE: Name: .text
# ONE: Address: 0x200
# SIZE255: Name: .text
# SIZE255: Address: 0x200
# SIZE256: Name: .text
# SIZE256: Address: 0x200
# SIZE257: Name: .text
# SIZE257: Address: 0x300
# SIZE1024: Name: .data
# SIZE1024: Size: 1024
# SIZE1024: Name: .text
# SIZE1024: Address: 0x500
# SIZE1025: AVM static storage exceeds the 1024-byte limit
# RELAX: Entry: 0x102
# RELAX: Name: _start
# RELAX-NEXT: Value: 0x102
# MISSING: AVM entry symbol _start is undefined
# BADENTRY: AVM entry symbol {{(_start|saved_entry)}} must be in an executable program-space section
# GC: Name: .saved
# GC: Address: 0x100
# GC: Size: 1
# GC: Name: .data
# GC: Address: 0x101
# GC: Size: 1
# GC: Name: .text
# GC: Address: 0x200
# GC: Name: live_saved
# GC: Name: live_data
# GC-NOT: Name: dead_saved
# GC-NOT: Name: dead_data
# MULTI: Entry: 0x200
# MULTI: Name: .saved
# MULTI: Address: 0x100
# MULTI: Name: .data
# MULTI: Address: 0x101
# MULTI: Name: function
# MULTI: Value: 0x20C
# MULTI: Name: moved
# MULTI: Value: 0x20D
# MULTIBYTES: Contents of section .text:
# MULTIBYTES: 0200 d40a0001 01010d02 00000000 00000000
# COMDAT: Name: .saved
# COMDAT: Size: 1
# COMDAT: Name: .data
# COMDAT: Size: 1
# COMDAT: Name: group_symbol
# COMDAT-NEXT: Value: 0x204
# GOOD: Entry: 0x300
# GOOD: Name: .saved
# GOOD: Address: 0x100
# GOOD: Name: .data
# GOOD: Address: 0x101
# GOOD: Name: .text
# GOOD: Address: 0x300
# BADSAVED: AVM .saved must begin at data address 0x100
# BADDATA: AVM .data must begin at data address 0x101
# HIGH: Entry: 0xFFFFFF
# CROSS: Name: .text
# CROSS: Address: 0xFFFF
# CROSS: Size: 3
# RESERVED: AVM program-space section .text overlaps the reserved header range
# OVERFLOW: AVM program-space section .text exceeds the 24-bit program-space limit

#--- layout.s
.text
.globl _start
_start:
  .byte 0
.section .text.more,"ax"
  .byte 1
.section .rodata.more,"a"
  .byte 2
.section .init_array.100,"a",@init_array
  .progptr _start
.section .fini_array.100,"a",@fini_array
  .progptr _start
.section .saved.state,"aw",@progbits
  .byte 3
.section .data.runtime,"aw",@progbits
  .byte 4

#--- empty.s
.text
.globl _start
_start:
  .byte 0

#--- one.s
.data
  .byte 0
.text
.globl _start
_start:
  .byte 0

#--- size255.s
.data
  .zero 255
.text
.globl _start
_start:
  .byte 0

#--- size256.s
.data
  .zero 256
.text
.globl _start
_start:
  .byte 0

#--- size257.s
.data
  .zero 257
.text
.globl _start
_start:
  .byte 0

#--- size1024.s
.data
  .zero 1024
.text
.globl _start
_start:
  .byte 0

#--- size1025.s
.data
  .zero 1025
.text
.globl _start
_start:
  .byte 0

#--- relax-entry.s
.text
  jmp target
.globl _start
_start:
  .byte 0
target:
  .byte 0

#--- missing.s
.text
not_start:
  .byte 0

#--- rodata-entry.s
.rodata
.globl _start
_start:
  .byte 0

#--- gc-start.s
.text
.globl _start
_start:
  .short live_saved
  .short live_data

#--- gc-live.s
.section .saved.live,"aw",@progbits
.globl live_saved
live_saved:
  .byte 0
.section .data.live,"aw",@progbits
.globl live_data
live_data:
  .byte 0

#--- gc-dead.s
.section .saved.dead,"aw",@progbits
.globl dead_saved
dead_saved:
  .byte 0
.section .data.dead,"aw",@progbits
.globl dead_data
dead_data:
  .byte 0
.section .text.dead,"ax"
  .byte 0
.section .rodata.dead,"a"
  .byte 0

#--- multi-a.s
.text
.globl _start
_start:
  jmp function
  .short state
  .short runtime
  .progptr moved

#--- multi-b.s
.section .text.function,"ax"
.globl function
function:
  .byte 0
.globl moved
moved:
  .byte 0
.section .rodata.constant,"a"
  .byte 0

#--- multi-c.s
.section .saved.state,"aw",@progbits
.globl state
state:
  .byte 0
.section .data.runtime,"aw",@progbits
.globl runtime
runtime:
  .byte 0

#--- comdat-start.s
.text
.globl _start
_start:
  jmp group_symbol

#--- comdat-a.s
.section .saved.group,"awG",@progbits,group_symbol,comdat
  .byte 1
.section .data.group,"awG",@progbits,group_symbol,comdat
  .byte 2
.section .text.group,"axG",@progbits,group_symbol,comdat
.globl group_symbol
group_symbol:
  .byte 3

#--- comdat-b.s
.section .saved.group,"awG",@progbits,group_symbol,comdat
  .byte 4
.section .data.group,"awG",@progbits,group_symbol,comdat
  .byte 5
.section .text.group,"axG",@progbits,group_symbol,comdat
.globl group_symbol
group_symbol:
  .byte 6

#--- script.s
.saved
.globl saved_entry
saved_entry:
  .byte 0
.data
  .byte 0
.text
.globl custom_entry
custom_entry:
  .byte 0

#--- good.ld
ENTRY(custom_entry)
SECTIONS { .saved 0x100 : { *(.saved) } .data 0x101 : { *(.data) } .text 0x300 : { *(.text) } }

#--- bad-saved.ld
ENTRY(custom_entry)
SECTIONS { .saved 0x101 : { *(.saved) } .data 0x102 : { *(.data) } .text 0x300 : { *(.text) } }

#--- bad-data.ld
ENTRY(custom_entry)
SECTIONS { .saved 0x100 : { *(.saved) } .data 0x102 : { *(.data) } .text 0x300 : { *(.text) } }

#--- bad-entry.ld
ENTRY(saved_entry)
SECTIONS { .saved 0x100 : { *(.saved) } .data 0x101 : { *(.data) } .text 0x300 : { *(.text) } }

#--- range.s
.text
.globl _start
_start:
  .byte 0
  .byte 0

#--- high.s
.section .text.high,"ax"
.globl _start
_start:
  .byte 0

#--- high.ld
ENTRY(_start)
SECTIONS { .text 0xffffff : { *(.text.high) } }

#--- cross.ld
ENTRY(_start)
SECTIONS { .text 0xffff : { *(.text) } }

#--- reserved.ld
ENTRY(_start)
SECTIONS { .text 0xff : { *(.text) } }

#--- overflow.ld
ENTRY(_start)
SECTIONS { .text 0x1000000 : { *(.text) } }
