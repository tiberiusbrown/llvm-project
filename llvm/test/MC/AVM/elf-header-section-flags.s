# RUN: llvm-mc -triple=avm -filetype=obj %s -o %t.o
# RUN: llvm-readobj --file-headers --sections --relocations %t.o | FileCheck %s

.text
.globl function
function:
  nop
  .short data_object

.rodata
constant:
  .byte 1

.data
data_object:
  .short 2
  .progptr function

.bss
  .zero 3

.section .init_array,"a",@init_array
  .progptr function

.section .fini_array,"a",@fini_array
  .progptr function

.section .avm.metadata
  .byte 0

.section .debug_info
  .byte 0

# CHECK: Format: elf32-avm
# CHECK: Arch: avm
# CHECK: AddressSize: 32bit
# CHECK: DataEncoding: LittleEndian
# CHECK: Type: Relocatable
# CHECK: Machine: 0x4156
# CHECK: Flags [ (0x1)

# CHECK: Name: .text
# CHECK: Type: SHT_PROGBITS
# CHECK: Flags [ (0x100006)
# CHECK: SHF_ALLOC
# CHECK: SHF_EXECINSTR
# CHECK: Name: .rela.text
# CHECK: Type: SHT_RELA
# CHECK: Flags [ (0x40)

# CHECK: Name: .rodata
# CHECK: Type: SHT_PROGBITS
# CHECK: Flags [ (0x100002)
# CHECK: SHF_ALLOC

# CHECK: Name: .data
# CHECK: Type: SHT_PROGBITS
# CHECK: Flags [ (0x200003)
# CHECK: SHF_ALLOC
# CHECK: SHF_WRITE
# CHECK: Name: .rela.data
# CHECK: Type: SHT_RELA
# CHECK: Flags [ (0x40)

# CHECK: Name: .bss
# CHECK: Type: SHT_NOBITS
# CHECK: Flags [ (0x200003)
# CHECK: SHF_ALLOC
# CHECK: SHF_WRITE

# CHECK: Name: .init_array
# CHECK: Type: SHT_INIT_ARRAY
# CHECK: Flags [ (0x100002)
# CHECK: SHF_ALLOC
# CHECK: EntrySize: 3

# CHECK: Name: .fini_array
# CHECK: Type: SHT_FINI_ARRAY
# CHECK: Flags [ (0x100002)
# CHECK: SHF_ALLOC
# CHECK: EntrySize: 3

# CHECK: Name: .avm.metadata
# CHECK: Type: SHT_PROGBITS
# CHECK: Flags [ (0x0)
# CHECK: Name: .debug_info
# CHECK: Flags [ (0x0)

# CHECK: Section ({{.*}}) .rela.init_array {
# CHECK: R_AVM_PROG24 function 0x0
# CHECK: Section ({{.*}}) .rela.fini_array {
# CHECK: R_AVM_PROG24 function 0x0
