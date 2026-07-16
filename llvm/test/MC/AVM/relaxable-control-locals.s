# RUN: llvm-mc -triple=avm -filetype=obj %s -o %t.o
# RUN: llvm-readobj --relocations %t.o | FileCheck %s

back:
jmp back
call forward
breq short_target
brne far_target
brult back
bruge forward
brslt short_target
brsge far_target
short_target:
.zero 130
far_target:
forward:

# Both relocation kinds are retained even for local, same-section targets.
# CHECK: 0x0 R_AVM_RELAX - 0x0
# CHECK: 0x1 R_AVM_FAR24 .text
# CHECK: 0x4 R_AVM_RELAX - 0x0
# CHECK: 0x5 R_AVM_FAR24 .text
# CHECK: 0x8 R_AVM_RELAX - 0x0
# CHECK: 0xB R_AVM_FAR24 .text
# CHECK: 0xE R_AVM_RELAX - 0x0
# CHECK: 0x11 R_AVM_FAR24 .text
# CHECK: 0x14 R_AVM_RELAX - 0x0
# CHECK: 0x17 R_AVM_FAR24 .text
# CHECK: 0x1A R_AVM_RELAX - 0x0
# CHECK: 0x1D R_AVM_FAR24 .text
# CHECK: 0x20 R_AVM_RELAX - 0x0
# CHECK: 0x23 R_AVM_FAR24 .text
# CHECK: 0x26 R_AVM_RELAX - 0x0
# CHECK: 0x29 R_AVM_FAR24 .text
