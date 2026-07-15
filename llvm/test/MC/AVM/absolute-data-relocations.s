# RUN: llvm-mc -triple=avm-unknown-arduboyfx -filetype=obj %s -o %t.o
# RUN: llvm-readobj --relocations %t.o | FileCheck %s

.text
.globl symbol
ldm8u r0, [symbol]
ldm8u r1, [symbol+1]
ldm8u r2, [symbol-1]
stm8 [symbol], r3
stm8 [symbol+1], r4
stm8 [symbol-1], r5
ldm16 r6, [symbol]
ldm16 r7, [symbol+1]
ldm16 r0, [symbol-1]
stm16 [symbol], r1
stm16 [symbol+1], r2
stm16 [symbol-1], r3
local_symbol:
ldm8u r4, [local_symbol]
.globl global_symbol
global_symbol:
stm8 [global_symbol], r5

# CHECK: 0x2 R_AVM_DATA16 symbol 0x0
# CHECK: 0x6 R_AVM_DATA16 symbol 0x1
# CHECK: 0xA R_AVM_DATA16 symbol 0xFFFFFFFF
# CHECK: 0xE R_AVM_DATA16 symbol 0x0
# CHECK: 0x12 R_AVM_DATA16 symbol 0x1
# CHECK: 0x16 R_AVM_DATA16 symbol 0xFFFFFFFF
# CHECK: 0x1A R_AVM_DATA16 symbol 0x0
# CHECK: 0x1E R_AVM_DATA16 symbol 0x1
# CHECK: 0x22 R_AVM_DATA16 symbol 0xFFFFFFFF
# CHECK: 0x26 R_AVM_DATA16 symbol 0x0
# CHECK: 0x2A R_AVM_DATA16 symbol 0x1
# CHECK: 0x2E R_AVM_DATA16 symbol 0xFFFFFFFF
# CHECK: 0x32 R_AVM_DATA16 .text 0x30
# CHECK: 0x36 R_AVM_DATA16 global_symbol 0x0
