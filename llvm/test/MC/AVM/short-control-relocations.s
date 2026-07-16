# RUN: llvm-mc -triple=avm -filetype=obj %s -o %t.o
# RUN: llvm-readobj --relocations %t.o | FileCheck %s

breq8 symbol
brne8 symbol + 1
brult8 symbol - 1
brslt8 symbol
bruge8 symbol + 1
brsge8 symbol - 1
jmp8 symbol + 1
call8 symbol

# CHECK: 0x1 R_AVM_PCREL8 symbol 0x0
# CHECK: 0x3 R_AVM_PCREL8 symbol 0x1
# CHECK: 0x5 R_AVM_PCREL8 symbol 0xFFFFFFFF
# CHECK: 0x7 R_AVM_PCREL8 symbol 0x0
# CHECK: 0x9 R_AVM_PCREL8 symbol 0x1
# CHECK: 0xB R_AVM_PCREL8 symbol 0xFFFFFFFF
# CHECK: 0xD R_AVM_PCREL8 symbol 0x1
# CHECK: 0xF R_AVM_PCREL8 symbol 0x0
# CHECK-NOT: R_AVM_RELAX
