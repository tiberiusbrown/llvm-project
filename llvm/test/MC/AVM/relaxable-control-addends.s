# RUN: llvm-mc -triple=avm -filetype=obj %s -o %t.o
# RUN: llvm-readobj --relocations %t.o | FileCheck %s

jmp target+1
call target-2
breq target+3
brne target-4
brult target+5
bruge target-6
brslt target+7
brsge target-8

# CHECK: 0x0 R_AVM_RELAX - 0x0
# CHECK: 0x1 R_AVM_FAR24 target 0x1
# CHECK: 0x4 R_AVM_RELAX - 0x0
# CHECK: 0x5 R_AVM_FAR24 target 0xFFFFFFFE
# CHECK: 0x8 R_AVM_RELAX - 0x0
# CHECK: 0xB R_AVM_FAR24 target 0x3
# CHECK: 0xE R_AVM_RELAX - 0x0
# CHECK: 0x11 R_AVM_FAR24 target 0xFFFFFFFC
# CHECK: 0x14 R_AVM_RELAX - 0x0
# CHECK: 0x17 R_AVM_FAR24 target 0x5
# CHECK: 0x1A R_AVM_RELAX - 0x0
# CHECK: 0x1D R_AVM_FAR24 target 0xFFFFFFFA
# CHECK: 0x20 R_AVM_RELAX - 0x0
# CHECK: 0x23 R_AVM_FAR24 target 0x7
# CHECK: 0x26 R_AVM_RELAX - 0x0
# CHECK: 0x29 R_AVM_FAR24 target 0xFFFFFFF8
