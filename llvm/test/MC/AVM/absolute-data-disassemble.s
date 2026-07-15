# RUN: llvm-mc -triple=avm --disassemble < %S/Inputs/absolute-data.txt | FileCheck %s
# RUN: llvm-mc -triple=avm --disassemble < %S/Inputs/absolute-data-truncated-f0.txt 2>&1 | FileCheck --check-prefix=TRUNC %s
# RUN: llvm-mc -triple=avm --disassemble < %S/Inputs/absolute-data-truncated-secondary.txt 2>&1 | FileCheck --check-prefix=TRUNC %s
# RUN: llvm-mc -triple=avm --disassemble < %S/Inputs/absolute-data-truncated-address.txt 2>&1 | FileCheck --check-prefix=TRUNC %s

# CHECK: ldm8u r0, [0]
# CHECK: ldm8u r1, [1]
# CHECK: ldm8u r2, [255]
# CHECK: ldm8u r3, [256]
# CHECK: ldm8u r4, [4660]
# CHECK: ldm8u r5, [65535]
# CHECK: ldm8u r6, [1]
# CHECK: ldm8u r7, [256]
# CHECK: stm8 [0], r0
# CHECK: stm8 [1], r1
# CHECK: stm8 [255], r2
# CHECK: stm8 [256], r3
# CHECK: stm8 [4660], r4
# CHECK: stm8 [65535], r5
# CHECK: stm8 [1], r6
# CHECK: stm8 [256], r7
# CHECK: ldm16 r0, [0]
# CHECK: ldm16 r1, [1]
# CHECK: ldm16 r2, [255]
# CHECK: ldm16 r3, [256]
# CHECK: ldm16 r4, [4660]
# CHECK: ldm16 r5, [65535]
# CHECK: ldm16 r6, [1]
# CHECK: ldm16 r7, [256]
# CHECK: stm16 [0], r0
# CHECK: stm16 [1], r1
# CHECK: stm16 [255], r2
# CHECK: stm16 [256], r3
# CHECK: stm16 [4660], r4
# CHECK: stm16 [65535], r5
# CHECK: stm16 [1], r6
# CHECK: stm16 [256], r7
# CHECK: mov c0, c0
# CHECK: ldi8 c0, 18
# TRUNC: warning: invalid instruction encoding
