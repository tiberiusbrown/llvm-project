# RUN: llvm-mc -triple=avm --show-encoding %s | FileCheck %s --check-prefix=CHECK
# RUN: llvm-mc -triple=avm --disassemble < %S/Inputs/fe-valid.txt | FileCheck %s --check-prefix=DIS
# RUN: llvm-mc -triple=avm -filetype=obj %s -o %t.o
# RUN: llvm-objdump -d --no-show-raw-insn %t.o | FileCheck %s --check-prefix=OBJ
# RUN: llvm-readobj --relocations %t.o | FileCheck %s --check-prefix=NORELOC
# RUN: llvm-mc -triple=avm -filetype=obj %s -o %t.orig.o
# RUN: llvm-objdump -d --no-show-raw-insn %t.orig.o | sed -n '/^[[:space:]]*[0-9a-f][0-9a-f]*:/s/.*:[[:space:]]*//p' > %t.reassembled.s
# RUN: llvm-mc -triple=avm -filetype=obj %t.reassembled.s -o %t.reassembled.o
# RUN: llvm-objdump -s --section=.text %t.orig.o | grep '^[[:space:]]*0000' > %t.orig.txt
# RUN: llvm-objdump -s --section=.text %t.reassembled.o | grep '^[[:space:]]*0000' > %t.reassembled.txt
# RUN: diff -u %t.orig.txt %t.reassembled.txt
# DIS-COUNT-64: mul16
# OBJ-COUNT-64: mul16
# NORELOC: Relocations [
# NORELOC-NEXT: ]
mul16 r0, r0
# CHECK: mul16 r0, r0{{.*}}encoding: [0xfe,0x00]
mul16 r0, r1
# CHECK: mul16 r0, r1{{.*}}encoding: [0xfe,0x01]
mul16 r0, r2
# CHECK: mul16 r0, r2{{.*}}encoding: [0xfe,0x02]
mul16 r0, r3
# CHECK: mul16 r0, r3{{.*}}encoding: [0xfe,0x03]
mul16 r0, r4
# CHECK: mul16 r0, r4{{.*}}encoding: [0xfe,0x04]
mul16 r0, r5
# CHECK: mul16 r0, r5{{.*}}encoding: [0xfe,0x05]
mul16 r0, r6
# CHECK: mul16 r0, r6{{.*}}encoding: [0xfe,0x06]
mul16 r0, r7
# CHECK: mul16 r0, r7{{.*}}encoding: [0xfe,0x07]
mul16 r1, r0
# CHECK: mul16 r1, r0{{.*}}encoding: [0xfe,0x08]
mul16 r1, r1
# CHECK: mul16 r1, r1{{.*}}encoding: [0xfe,0x09]
mul16 r1, r2
# CHECK: mul16 r1, r2{{.*}}encoding: [0xfe,0x0a]
mul16 r1, r3
# CHECK: mul16 r1, r3{{.*}}encoding: [0xfe,0x0b]
mul16 r1, r4
# CHECK: mul16 r1, r4{{.*}}encoding: [0xfe,0x0c]
mul16 r1, r5
# CHECK: mul16 r1, r5{{.*}}encoding: [0xfe,0x0d]
mul16 r1, r6
# CHECK: mul16 r1, r6{{.*}}encoding: [0xfe,0x0e]
mul16 r1, r7
# CHECK: mul16 r1, r7{{.*}}encoding: [0xfe,0x0f]
mul16 r2, r0
# CHECK: mul16 r2, r0{{.*}}encoding: [0xfe,0x10]
mul16 r2, r1
# CHECK: mul16 r2, r1{{.*}}encoding: [0xfe,0x11]
mul16 r2, r2
# CHECK: mul16 r2, r2{{.*}}encoding: [0xfe,0x12]
mul16 r2, r3
# CHECK: mul16 r2, r3{{.*}}encoding: [0xfe,0x13]
mul16 r2, r4
# CHECK: mul16 r2, r4{{.*}}encoding: [0xfe,0x14]
mul16 r2, r5
# CHECK: mul16 r2, r5{{.*}}encoding: [0xfe,0x15]
mul16 r2, r6
# CHECK: mul16 r2, r6{{.*}}encoding: [0xfe,0x16]
mul16 r2, r7
# CHECK: mul16 r2, r7{{.*}}encoding: [0xfe,0x17]
mul16 r3, r0
# CHECK: mul16 r3, r0{{.*}}encoding: [0xfe,0x18]
mul16 r3, r1
# CHECK: mul16 r3, r1{{.*}}encoding: [0xfe,0x19]
mul16 r3, r2
# CHECK: mul16 r3, r2{{.*}}encoding: [0xfe,0x1a]
mul16 r3, r3
# CHECK: mul16 r3, r3{{.*}}encoding: [0xfe,0x1b]
mul16 r3, r4
# CHECK: mul16 r3, r4{{.*}}encoding: [0xfe,0x1c]
mul16 r3, r5
# CHECK: mul16 r3, r5{{.*}}encoding: [0xfe,0x1d]
mul16 r3, r6
# CHECK: mul16 r3, r6{{.*}}encoding: [0xfe,0x1e]
mul16 r3, r7
# CHECK: mul16 r3, r7{{.*}}encoding: [0xfe,0x1f]
mul16 r4, r0
# CHECK: mul16 r4, r0{{.*}}encoding: [0xfe,0x20]
mul16 r4, r1
# CHECK: mul16 r4, r1{{.*}}encoding: [0xfe,0x21]
mul16 r4, r2
# CHECK: mul16 r4, r2{{.*}}encoding: [0xfe,0x22]
mul16 r4, r3
# CHECK: mul16 r4, r3{{.*}}encoding: [0xfe,0x23]
mul16 r4, r4
# CHECK: mul16 r4, r4{{.*}}encoding: [0xfe,0x24]
mul16 r4, r5
# CHECK: mul16 r4, r5{{.*}}encoding: [0xfe,0x25]
mul16 r4, r6
# CHECK: mul16 r4, r6{{.*}}encoding: [0xfe,0x26]
mul16 r4, r7
# CHECK: mul16 r4, r7{{.*}}encoding: [0xfe,0x27]
mul16 r5, r0
# CHECK: mul16 r5, r0{{.*}}encoding: [0xfe,0x28]
mul16 r5, r1
# CHECK: mul16 r5, r1{{.*}}encoding: [0xfe,0x29]
mul16 r5, r2
# CHECK: mul16 r5, r2{{.*}}encoding: [0xfe,0x2a]
mul16 r5, r3
# CHECK: mul16 r5, r3{{.*}}encoding: [0xfe,0x2b]
mul16 r5, r4
# CHECK: mul16 r5, r4{{.*}}encoding: [0xfe,0x2c]
mul16 r5, r5
# CHECK: mul16 r5, r5{{.*}}encoding: [0xfe,0x2d]
mul16 r5, r6
# CHECK: mul16 r5, r6{{.*}}encoding: [0xfe,0x2e]
mul16 r5, r7
# CHECK: mul16 r5, r7{{.*}}encoding: [0xfe,0x2f]
mul16 r6, r0
# CHECK: mul16 r6, r0{{.*}}encoding: [0xfe,0x30]
mul16 r6, r1
# CHECK: mul16 r6, r1{{.*}}encoding: [0xfe,0x31]
mul16 r6, r2
# CHECK: mul16 r6, r2{{.*}}encoding: [0xfe,0x32]
mul16 r6, r3
# CHECK: mul16 r6, r3{{.*}}encoding: [0xfe,0x33]
mul16 r6, r4
# CHECK: mul16 r6, r4{{.*}}encoding: [0xfe,0x34]
mul16 r6, r5
# CHECK: mul16 r6, r5{{.*}}encoding: [0xfe,0x35]
mul16 r6, r6
# CHECK: mul16 r6, r6{{.*}}encoding: [0xfe,0x36]
mul16 r6, r7
# CHECK: mul16 r6, r7{{.*}}encoding: [0xfe,0x37]
mul16 r7, r0
# CHECK: mul16 r7, r0{{.*}}encoding: [0xfe,0x38]
mul16 r7, r1
# CHECK: mul16 r7, r1{{.*}}encoding: [0xfe,0x39]
mul16 r7, r2
# CHECK: mul16 r7, r2{{.*}}encoding: [0xfe,0x3a]
mul16 r7, r3
# CHECK: mul16 r7, r3{{.*}}encoding: [0xfe,0x3b]
mul16 r7, r4
# CHECK: mul16 r7, r4{{.*}}encoding: [0xfe,0x3c]
mul16 r7, r5
# CHECK: mul16 r7, r5{{.*}}encoding: [0xfe,0x3d]
mul16 r7, r6
# CHECK: mul16 r7, r6{{.*}}encoding: [0xfe,0x3e]
mul16 r7, r7
# CHECK: mul16 r7, r7{{.*}}encoding: [0xfe,0x3f]
