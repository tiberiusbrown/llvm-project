# RUN: llvm-mc -triple=avm-unknown-arduboyfx -filetype=obj %s -o %t.o
# RUN: llvm-readobj --file-headers --sections %t.o | FileCheck %s

.text
.globl text_symbol
text_symbol:
  .byte 0

.section .rodata
.byte 0

.data
.byte 0

.saved
.zero 1

.section .init_array,"a",@init_array
.progptr text_symbol

.section .fini_array,"a",@fini_array
.progptr text_symbol

# CHECK: Arch: avm
# CHECK: Flags [ (0x1)
# CHECK: Name: .text
# CHECK: Flags [ (0x10000006)
# CHECK:   SHF_ALLOC
# CHECK:   SHF_AVM_PROGSPACE
# CHECK:   SHF_EXECINSTR
# CHECK: Name: .rodata
# CHECK: Flags [ (0x10000002)
# CHECK:   SHF_ALLOC
# CHECK:   SHF_AVM_PROGSPACE
# CHECK: Name: .data
# CHECK: Flags [ (0x20000003)
# CHECK:   SHF_ALLOC
# CHECK:   SHF_AVM_DATASPACE
# CHECK:   SHF_WRITE
# CHECK-NOT: SHF_GNU_RETAIN
# CHECK: Name: .saved
# CHECK: Flags [ (0x20000003)
# CHECK:   SHF_ALLOC
# CHECK:   SHF_AVM_DATASPACE
# CHECK:   SHF_WRITE
# CHECK-NOT: SHF_GNU_RETAIN
# CHECK: Name: .init_array
# CHECK: Flags [ (0x10000002)
# CHECK:   SHF_ALLOC
# CHECK:   SHF_AVM_PROGSPACE
# CHECK: EntrySize: 3
# CHECK: Name: .fini_array
# CHECK: Flags [ (0x10000002)
# CHECK:   SHF_ALLOC
# CHECK:   SHF_AVM_PROGSPACE
# CHECK: EntrySize: 3
