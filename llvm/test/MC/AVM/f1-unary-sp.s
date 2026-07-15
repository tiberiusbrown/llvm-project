# RUN: llvm-mc -triple=avm -show-encoding %s | FileCheck %s
# RUN: llvm-mc -triple=avm -filetype=obj %s -o %t.o
# RUN: llvm-objdump -d --no-show-raw-insn %t.o | FileCheck --check-prefix=DIS %s
# RUN: llvm-objdump -d --no-show-raw-insn %t.o | sed -n '/^[[:space:]]*[0-9a-f][0-9a-f]*:/s/.*:[[:space:]]*//p' > %t.reassembled.s
# RUN: llvm-mc -triple=avm -filetype=obj %t.reassembled.s -o %t.reassembled.o
# RUN: llvm-objdump -s --section=.text %t.o | grep '^[[:space:]]*0000' > %t.orig.txt
# RUN: llvm-objdump -s --section=.text %t.reassembled.o | grep '^[[:space:]]*0000' > %t.reassembled.txt
# RUN: diff -u %t.orig.txt %t.reassembled.txt

zext8 r0
zext8 r1
zext8 r2
zext8 r3
zext8 r4
zext8 r5
zext8 r6
zext8 r7
swap8 r0
swap8 r1
swap8 r2
swap8 r3
swap8 r4
swap8 r5
swap8 r6
swap8 r7
getsp r0
getsp r1
getsp r2
getsp r3
getsp r4
getsp r5
getsp r6
getsp r7
setsp r0
setsp r1
setsp r2
setsp r3
setsp r4
setsp r5
setsp r6
setsp r7

# CHECK: zext8 r0{{.*}}encoding: [0xf1,0x70]
# CHECK: zext8 r1{{.*}}encoding: [0xf1,0x71]
# CHECK: zext8 r2{{.*}}encoding: [0xf1,0x72]
# CHECK: zext8 r3{{.*}}encoding: [0xf1,0x73]
# CHECK: zext8 r4{{.*}}encoding: [0xf1,0x74]
# CHECK: zext8 r5{{.*}}encoding: [0xf1,0x75]
# CHECK: zext8 r6{{.*}}encoding: [0xf1,0x76]
# CHECK: zext8 r7{{.*}}encoding: [0xf1,0x77]
# CHECK: swap8 r0{{.*}}encoding: [0xf1,0x78]
# CHECK: swap8 r1{{.*}}encoding: [0xf1,0x79]
# CHECK: swap8 r2{{.*}}encoding: [0xf1,0x7a]
# CHECK: swap8 r3{{.*}}encoding: [0xf1,0x7b]
# CHECK: swap8 r4{{.*}}encoding: [0xf1,0x7c]
# CHECK: swap8 r5{{.*}}encoding: [0xf1,0x7d]
# CHECK: swap8 r6{{.*}}encoding: [0xf1,0x7e]
# CHECK: swap8 r7{{.*}}encoding: [0xf1,0x7f]
# CHECK: getsp r0{{.*}}encoding: [0xf1,0x80]
# CHECK: getsp r1{{.*}}encoding: [0xf1,0x81]
# CHECK: getsp r2{{.*}}encoding: [0xf1,0x82]
# CHECK: getsp r3{{.*}}encoding: [0xf1,0x83]
# CHECK: getsp r4{{.*}}encoding: [0xf1,0x84]
# CHECK: getsp r5{{.*}}encoding: [0xf1,0x85]
# CHECK: getsp r6{{.*}}encoding: [0xf1,0x86]
# CHECK: getsp r7{{.*}}encoding: [0xf1,0x87]
# CHECK: setsp r0{{.*}}encoding: [0xf1,0x88]
# CHECK: setsp r1{{.*}}encoding: [0xf1,0x89]
# CHECK: setsp r2{{.*}}encoding: [0xf1,0x8a]
# CHECK: setsp r3{{.*}}encoding: [0xf1,0x8b]
# CHECK: setsp r4{{.*}}encoding: [0xf1,0x8c]
# CHECK: setsp r5{{.*}}encoding: [0xf1,0x8d]
# CHECK: setsp r6{{.*}}encoding: [0xf1,0x8e]
# CHECK: setsp r7{{.*}}encoding: [0xf1,0x8f]

# DIS: zext8 r0
# DIS: zext8 r7
# DIS: swap8 r0
# DIS: swap8 r7
# DIS: getsp r0
# DIS: getsp r7
# DIS: setsp r0
# DIS: setsp r7
