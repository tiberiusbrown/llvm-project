# RUN: llvm-mc -triple=avm-unknown-arduboyfx -filetype=obj %s -o %t.o
# RUN: llvm-readobj --relocations %t.o | FileCheck %s

.text
.globl local_target
.globl external_target
jmpf local_target
callf external_target + 4
local_target:

# CHECK: 0x1 R_AVM_FAR24 local_target 0x0
# CHECK: 0x5 R_AVM_FAR24 external_target 0x4
