# RUN: llvm-mc -triple=avm -filetype=obj %s -o %t.o
# RUN: llvm-readobj --relocations %t.o | FileCheck %s

breq symbol
brne symbol + 1
brult symbol - 1
brslt symbol
jmp symbol + 1

# CHECK: 0x1 R_AVM_PCREL8 symbol 0x0
# CHECK: 0x3 R_AVM_PCREL8 symbol 0x1
# CHECK: 0x5 R_AVM_PCREL8 symbol 0xFFFFFFFF
# CHECK: 0x7 R_AVM_PCREL8 symbol 0x0
# CHECK: 0x9 R_AVM_PCREL8 symbol 0x1
# CHECK-NOT: R_AVM_RELAX
