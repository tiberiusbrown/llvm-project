# RUN: not llvm-mc -triple=avm -show-encoding %s 2>&1 | FileCheck %s

ldi8 r4, 1
ldi8 q0, 1
ldi8 r0, -1
ldi8 r0, 256
ldi8 r0, symbol
ldi16 r4, 1
ldi16 r0, -1
ldi16 r0, 65536
ldi16 r0, symbol
addi.s8 r4, 1
addi.s8 r0, -129
addi.s8 r0, 128
addi.s8 r0, 0xff
addi.s8 r0, symbol
cmpi.s8 r4, 1
cmpi.s8 r0, -129
cmpi.s8 r0, 128
cmpi.s8 r0, 0xff
cmpi.s8 r0, symbol
leasp c0, 1
leasp q0, 1
leasp r0, -1
leasp r0, 256
leasp r0, symbol
ldsp8u c0, [sp+0]
ldsp8u r0, [sp]
ldsp8u r0, [sp-1]
ldsp8u r0, [sp+256]
ldsp8u r0, [sp+symbol]
ldsp8u r0, 0
ldsp8s c0, [sp+0]
ldsp8s r0, [sp]
ldsp8s r0, [sp-1]
ldsp8s r0, [sp+256]
ldsp8s r0, [sp+symbol]
ldsp16 c0, [sp+0]
ldsp16 r0, [sp]
ldsp16 r0, [sp-1]
ldsp16 r0, [sp+256]
ldsp16 r0, [sp+symbol]
stsp8 r0, [sp+0]
stsp8 [sp+0]
stsp8 [sp+0], r0, r1
stsp8 [sp+0], q0
stsp8 [sp+0], sp
stsp8 [sp+0], pc
stsp8 [sp+0], cc
stsp8 [sp-1], c0
stsp8 [sp+16], c0
stsp8 [sp+255], c0
stsp8 [sp+symbol], c0
stsp8 c0, [sp+0]
stsp8
stsp8 [sp+0]
stsp8 [sp+0], c0, c1
stsp16 r0, [sp+0]
stsp16 [sp+0], c0
stsp16 [sp+0]
stsp16 [sp+0], r0, r1

# Every malformed form above is rejected.
# CHECK: error:
