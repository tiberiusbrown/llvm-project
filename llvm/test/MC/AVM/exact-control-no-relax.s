# RUN: llvm-mc -triple=avm -filetype=obj %s -o %t.o
# RUN: llvm-readobj --relocations %t.o | FileCheck %s

breq8 target
brne8 target
brult8 target
bruge8 target
brslt8 target
brsge8 target
breq16 target
brne16 target
brult16 target
bruge16 target
brslt16 target
brsge16 target
jmp8 target
call8 target
jmp16 target
call16 target
jmpf target
callf target

# CHECK-COUNT-6: R_AVM_PCREL8 target 0x0
# CHECK-COUNT-6: R_AVM_PCREL16 target 0x0
# CHECK-COUNT-2: R_AVM_PCREL8 target 0x0
# CHECK-COUNT-2: R_AVM_PCREL16 target 0x0
# CHECK-COUNT-2: R_AVM_FAR24 target 0x0
# CHECK-NOT: R_AVM_RELAX
