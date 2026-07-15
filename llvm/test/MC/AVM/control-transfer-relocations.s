# RUN: llvm-mc -triple=avm -filetype=obj %s -o %t.o
# RUN: llvm-readobj --relocations %t.o | FileCheck %s

jmp16 symbol
jmp16 symbol + 1
jmp16 symbol - 1
call16 symbol
call16 symbol + 1
call16 symbol - 1
jmpf symbol + 1
callf symbol - 1

# CHECK: 0x1 R_AVM_PCREL16 symbol 0x0
# CHECK: 0x4 R_AVM_PCREL16 symbol 0x1
# CHECK: 0x7 R_AVM_PCREL16 symbol 0xFFFFFFFF
# CHECK: 0xA R_AVM_PCREL16 symbol 0x0
# CHECK: 0xD R_AVM_PCREL16 symbol 0x1
# CHECK: 0x10 R_AVM_PCREL16 symbol 0xFFFFFFFF
# CHECK: 0x13 R_AVM_FAR24 symbol 0x1
# CHECK: 0x17 R_AVM_FAR24 symbol 0xFFFFFFFF
# CHECK-NOT: R_AVM_RELAX
