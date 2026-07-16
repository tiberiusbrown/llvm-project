# RUN: split-file %s %t
# RUN: llvm-mc -triple=avm -filetype=obj %t/code.s -o %t/code.o
# RUN: llvm-mc -triple=avm -filetype=obj %t/refs.s -o %t/refs.o
# RUN: ld.lld -e entry -T %t/layout.ld %t/code.o %t/refs.o -o %t/symbols.out
# RUN: llvm-readobj --file-headers --symbols %t/symbols.out | FileCheck %s --check-prefix=SYMBOLS
# RUN: llvm-objdump -s -j .text -j .data -j .meta %t/symbols.out | FileCheck %s --check-prefix=BYTES

# Three 4->2 relaxations move symbols after the sites by six bytes. The
# externally-defined metadata relocations must use the stabilized addresses.
# SYMBOLS: Entry: 0x123450
# SYMBOLS: Name: at_site
# SYMBOLS-NEXT: Value: 0x123450
# SYMBOLS: Name: moved
# SYMBOLS-NEXT: Value: 0x123456
# SYMBOLS-NEXT: Size: 1
# SYMBOLS: Name: between
# SYMBOLS-NEXT: Value: 0x123452
# SYMBOLS: Name: after_sites
# SYMBOLS-NEXT: Value: 0x123454
# SYMBOLS: Name: data_symbol
# SYMBOLS-NEXT: Value: 0x100
# SYMBOLS: Type: Object
# BYTES: Contents of section .text:
# BYTES-NEXT: 123450 d404d402 d40000
# BYTES: Contents of section .data:
# BYTES-NEXT: 0100 00
# BYTES: Contents of section .meta:
# BYTES-NEXT: 0200 56341256 34120001

#--- code.s
.text
.globl entry
.type entry,@function
entry:
.globl at_site
at_site:
jmp moved
.globl between
between:
jmp moved
.globl after_sites
after_sites:
jmp moved
.globl moved
.type moved,@function
moved:
.byte 0
.size moved, .-moved

.data
.globl data_symbol
.type data_symbol,@object
data_symbol:
.byte 0

#--- refs.s
.section .rodata.meta
.progptr moved
.short %lo16(moved)
.byte %hi8(moved)
.short data_symbol

#--- layout.ld
SECTIONS {
  .text 0x123450 : { *(.text) }
  .data 0x100 : { *(.data) }
  .meta 0x200 : { *(.rodata.meta) }
}
