# RUN: llvm-mc -triple=avm -filetype=obj %s -o %t.o
# RUN: llvm-readobj --relocations %t.o | FileCheck %s

breq target
brne target
brult target
bruge target
brslt target
brsge target
jmp8 target
call8 target
jmp16 target
call16 target
jmpf target
callf target

# CHECK-COUNT-8: R_AVM_PCREL8 target 0x0
# CHECK-COUNT-2: R_AVM_PCREL16 target 0x0
# CHECK-COUNT-2: R_AVM_FAR24 target 0x0
# CHECK-NOT: R_AVM_RELAX
