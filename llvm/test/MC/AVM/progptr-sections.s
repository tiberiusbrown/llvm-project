# RUN: llvm-mc -triple=avm-unknown-arduboyfx -filetype=obj %s -o %t.o
# RUN: llvm-readobj --sections --relocations %t.o | FileCheck %s

.section .init_array,"a",@init_array
.progptr ctor
.section .fini_array,"a",@fini_array
.progptr dtor

# CHECK: Name: .init_array
# CHECK: Type: SHT_INIT_ARRAY
# CHECK: Size: 3
# CHECK: EntrySize: 3
# CHECK: Name: .fini_array
# CHECK: Type: SHT_FINI_ARRAY
# CHECK: Size: 3
# CHECK: EntrySize: 3
# CHECK: R_AVM_PROG24 ctor 0x0
# CHECK: R_AVM_PROG24 dtor 0x0
