# RUN: llvm-mc -triple=avm --disassemble < %S/Inputs/cold-page.txt | FileCheck %s
# RUN: llvm-mc -triple=avm --disassemble < %S/Inputs/cold-page-truncated-f0.txt 2>&1 | FileCheck --check-prefix=TRUNC %s
# RUN: llvm-mc -triple=avm --disassemble < %S/Inputs/cold-page-truncated-ldi16.txt 2>&1 | FileCheck --check-prefix=TRUNC %s

# CHECK: ldi8 r0, 0
# CHECK: ldi8 r1, 1
# CHECK: ldi8 r2, 127
# CHECK: ldi8 r3, 255
# CHECK: ldi16 r0, 0
# CHECK: ldi16 r1, 1
# CHECK: ldi16 r2, 4660
# CHECK: ldi16 r3, 65535
# CHECK: addi.s8 r0, -128
# CHECK: addi.s8 r1, -1
# CHECK: addi.s8 r2, 0
# CHECK: addi.s8 r3, 127
# CHECK: cmpi.s8 r0, -127
# CHECK: cmpi.s8 r1, -1
# CHECK: cmpi.s8 r2, 1
# CHECK: cmpi.s8 r3, 126
# CHECK: leasp r0, 0
# CHECK: leasp r4, 255
# CHECK: ldsp8u r0, [sp+0]
# CHECK: ldsp8u r4, [sp+255]
# CHECK: ldsp8s r0, [sp+0]
# CHECK: ldsp8s r4, [sp+255]
# CHECK: stsp8 [sp+0], r0
# CHECK: stsp8 [sp+255], r4
# CHECK: ldsp16 r0, [sp+0]
# CHECK: ldsp16 r4, [sp+255]
# CHECK: stsp16 [sp+0], r0
# CHECK: stsp16 [sp+255], r4
# TRUNC: warning: invalid instruction encoding
