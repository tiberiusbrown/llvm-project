# RUN: llvm-mc -triple=avm --show-encoding %s | FileCheck %s --check-prefix=ENC
# RUN: llvm-mc -triple=avm --disassemble < %S/Inputs/ec-divrem-valid.txt | FileCheck %s --check-prefix=DIS
# RUN: llvm-mc -triple=avm -filetype=obj %s -o %t.o
# RUN: llvm-objdump -d --no-show-raw-insn %t.o | FileCheck %s --check-prefix=OBJ
# RUN: llvm-readobj --relocations %t.o | FileCheck %s --check-prefix=NORELOC
# RUN: llvm-mc -triple=avm -filetype=obj %s -o %t.orig.o
# RUN: llvm-objdump -d --no-show-raw-insn %t.orig.o | sed -n '/^[[:space:]]*[0-9a-f][0-9a-f]*:/s/.*:[[:space:]]*//p' > %t.reassembled.s
# RUN: llvm-mc -triple=avm -filetype=obj %t.reassembled.s -o %t.reassembled.o
# RUN: llvm-objdump -s --section=.text %t.orig.o | grep '^[[:space:]]*0000' > %t.orig.txt
# RUN: llvm-objdump -s --section=.text %t.reassembled.o | grep '^[[:space:]]*0000' > %t.reassembled.txt
# RUN: diff -u %t.orig.txt %t.reassembled.txt
# DIS-COUNT-64: udiv16
# DIS-COUNT-64: urem16
# DIS-COUNT-64: sdiv16
# DIS-COUNT-64: srem16
# OBJ-COUNT-64: udiv16
# OBJ-COUNT-64: urem16
# OBJ-COUNT-64: sdiv16
# OBJ-COUNT-64: srem16
# NORELOC: Relocations [
# NORELOC-NEXT: ]

# Exhaustive EC matrix: every operation selector and every rD/rS encoding.
udiv16 r0, r0
# ENC: udiv16 r0, r0{{.*}}encoding: [0xec,0x00]
udiv16 r0, r1
# ENC: udiv16 r0, r1{{.*}}encoding: [0xec,0x01]
udiv16 r0, r2
# ENC: udiv16 r0, r2{{.*}}encoding: [0xec,0x02]
udiv16 r0, r3
# ENC: udiv16 r0, r3{{.*}}encoding: [0xec,0x03]
udiv16 r0, r4
# ENC: udiv16 r0, r4{{.*}}encoding: [0xec,0x04]
udiv16 r0, r5
# ENC: udiv16 r0, r5{{.*}}encoding: [0xec,0x05]
udiv16 r0, r6
# ENC: udiv16 r0, r6{{.*}}encoding: [0xec,0x06]
udiv16 r0, r7
# ENC: udiv16 r0, r7{{.*}}encoding: [0xec,0x07]
udiv16 r1, r0
# ENC: udiv16 r1, r0{{.*}}encoding: [0xec,0x08]
udiv16 r1, r1
# ENC: udiv16 r1, r1{{.*}}encoding: [0xec,0x09]
udiv16 r1, r2
# ENC: udiv16 r1, r2{{.*}}encoding: [0xec,0x0a]
udiv16 r1, r3
# ENC: udiv16 r1, r3{{.*}}encoding: [0xec,0x0b]
udiv16 r1, r4
# ENC: udiv16 r1, r4{{.*}}encoding: [0xec,0x0c]
udiv16 r1, r5
# ENC: udiv16 r1, r5{{.*}}encoding: [0xec,0x0d]
udiv16 r1, r6
# ENC: udiv16 r1, r6{{.*}}encoding: [0xec,0x0e]
udiv16 r1, r7
# ENC: udiv16 r1, r7{{.*}}encoding: [0xec,0x0f]
udiv16 r2, r0
# ENC: udiv16 r2, r0{{.*}}encoding: [0xec,0x10]
udiv16 r2, r1
# ENC: udiv16 r2, r1{{.*}}encoding: [0xec,0x11]
udiv16 r2, r2
# ENC: udiv16 r2, r2{{.*}}encoding: [0xec,0x12]
udiv16 r2, r3
# ENC: udiv16 r2, r3{{.*}}encoding: [0xec,0x13]
udiv16 r2, r4
# ENC: udiv16 r2, r4{{.*}}encoding: [0xec,0x14]
udiv16 r2, r5
# ENC: udiv16 r2, r5{{.*}}encoding: [0xec,0x15]
udiv16 r2, r6
# ENC: udiv16 r2, r6{{.*}}encoding: [0xec,0x16]
udiv16 r2, r7
# ENC: udiv16 r2, r7{{.*}}encoding: [0xec,0x17]
udiv16 r3, r0
# ENC: udiv16 r3, r0{{.*}}encoding: [0xec,0x18]
udiv16 r3, r1
# ENC: udiv16 r3, r1{{.*}}encoding: [0xec,0x19]
udiv16 r3, r2
# ENC: udiv16 r3, r2{{.*}}encoding: [0xec,0x1a]
udiv16 r3, r3
# ENC: udiv16 r3, r3{{.*}}encoding: [0xec,0x1b]
udiv16 r3, r4
# ENC: udiv16 r3, r4{{.*}}encoding: [0xec,0x1c]
udiv16 r3, r5
# ENC: udiv16 r3, r5{{.*}}encoding: [0xec,0x1d]
udiv16 r3, r6
# ENC: udiv16 r3, r6{{.*}}encoding: [0xec,0x1e]
udiv16 r3, r7
# ENC: udiv16 r3, r7{{.*}}encoding: [0xec,0x1f]
udiv16 r4, r0
# ENC: udiv16 r4, r0{{.*}}encoding: [0xec,0x20]
udiv16 r4, r1
# ENC: udiv16 r4, r1{{.*}}encoding: [0xec,0x21]
udiv16 r4, r2
# ENC: udiv16 r4, r2{{.*}}encoding: [0xec,0x22]
udiv16 r4, r3
# ENC: udiv16 r4, r3{{.*}}encoding: [0xec,0x23]
udiv16 r4, r4
# ENC: udiv16 r4, r4{{.*}}encoding: [0xec,0x24]
udiv16 r4, r5
# ENC: udiv16 r4, r5{{.*}}encoding: [0xec,0x25]
udiv16 r4, r6
# ENC: udiv16 r4, r6{{.*}}encoding: [0xec,0x26]
udiv16 r4, r7
# ENC: udiv16 r4, r7{{.*}}encoding: [0xec,0x27]
udiv16 r5, r0
# ENC: udiv16 r5, r0{{.*}}encoding: [0xec,0x28]
udiv16 r5, r1
# ENC: udiv16 r5, r1{{.*}}encoding: [0xec,0x29]
udiv16 r5, r2
# ENC: udiv16 r5, r2{{.*}}encoding: [0xec,0x2a]
udiv16 r5, r3
# ENC: udiv16 r5, r3{{.*}}encoding: [0xec,0x2b]
udiv16 r5, r4
# ENC: udiv16 r5, r4{{.*}}encoding: [0xec,0x2c]
udiv16 r5, r5
# ENC: udiv16 r5, r5{{.*}}encoding: [0xec,0x2d]
udiv16 r5, r6
# ENC: udiv16 r5, r6{{.*}}encoding: [0xec,0x2e]
udiv16 r5, r7
# ENC: udiv16 r5, r7{{.*}}encoding: [0xec,0x2f]
udiv16 r6, r0
# ENC: udiv16 r6, r0{{.*}}encoding: [0xec,0x30]
udiv16 r6, r1
# ENC: udiv16 r6, r1{{.*}}encoding: [0xec,0x31]
udiv16 r6, r2
# ENC: udiv16 r6, r2{{.*}}encoding: [0xec,0x32]
udiv16 r6, r3
# ENC: udiv16 r6, r3{{.*}}encoding: [0xec,0x33]
udiv16 r6, r4
# ENC: udiv16 r6, r4{{.*}}encoding: [0xec,0x34]
udiv16 r6, r5
# ENC: udiv16 r6, r5{{.*}}encoding: [0xec,0x35]
udiv16 r6, r6
# ENC: udiv16 r6, r6{{.*}}encoding: [0xec,0x36]
udiv16 r6, r7
# ENC: udiv16 r6, r7{{.*}}encoding: [0xec,0x37]
udiv16 r7, r0
# ENC: udiv16 r7, r0{{.*}}encoding: [0xec,0x38]
udiv16 r7, r1
# ENC: udiv16 r7, r1{{.*}}encoding: [0xec,0x39]
udiv16 r7, r2
# ENC: udiv16 r7, r2{{.*}}encoding: [0xec,0x3a]
udiv16 r7, r3
# ENC: udiv16 r7, r3{{.*}}encoding: [0xec,0x3b]
udiv16 r7, r4
# ENC: udiv16 r7, r4{{.*}}encoding: [0xec,0x3c]
udiv16 r7, r5
# ENC: udiv16 r7, r5{{.*}}encoding: [0xec,0x3d]
udiv16 r7, r6
# ENC: udiv16 r7, r6{{.*}}encoding: [0xec,0x3e]
udiv16 r7, r7
# ENC: udiv16 r7, r7{{.*}}encoding: [0xec,0x3f]
urem16 r0, r0
# ENC: urem16 r0, r0{{.*}}encoding: [0xec,0x40]
urem16 r0, r1
# ENC: urem16 r0, r1{{.*}}encoding: [0xec,0x41]
urem16 r0, r2
# ENC: urem16 r0, r2{{.*}}encoding: [0xec,0x42]
urem16 r0, r3
# ENC: urem16 r0, r3{{.*}}encoding: [0xec,0x43]
urem16 r0, r4
# ENC: urem16 r0, r4{{.*}}encoding: [0xec,0x44]
urem16 r0, r5
# ENC: urem16 r0, r5{{.*}}encoding: [0xec,0x45]
urem16 r0, r6
# ENC: urem16 r0, r6{{.*}}encoding: [0xec,0x46]
urem16 r0, r7
# ENC: urem16 r0, r7{{.*}}encoding: [0xec,0x47]
urem16 r1, r0
# ENC: urem16 r1, r0{{.*}}encoding: [0xec,0x48]
urem16 r1, r1
# ENC: urem16 r1, r1{{.*}}encoding: [0xec,0x49]
urem16 r1, r2
# ENC: urem16 r1, r2{{.*}}encoding: [0xec,0x4a]
urem16 r1, r3
# ENC: urem16 r1, r3{{.*}}encoding: [0xec,0x4b]
urem16 r1, r4
# ENC: urem16 r1, r4{{.*}}encoding: [0xec,0x4c]
urem16 r1, r5
# ENC: urem16 r1, r5{{.*}}encoding: [0xec,0x4d]
urem16 r1, r6
# ENC: urem16 r1, r6{{.*}}encoding: [0xec,0x4e]
urem16 r1, r7
# ENC: urem16 r1, r7{{.*}}encoding: [0xec,0x4f]
urem16 r2, r0
# ENC: urem16 r2, r0{{.*}}encoding: [0xec,0x50]
urem16 r2, r1
# ENC: urem16 r2, r1{{.*}}encoding: [0xec,0x51]
urem16 r2, r2
# ENC: urem16 r2, r2{{.*}}encoding: [0xec,0x52]
urem16 r2, r3
# ENC: urem16 r2, r3{{.*}}encoding: [0xec,0x53]
urem16 r2, r4
# ENC: urem16 r2, r4{{.*}}encoding: [0xec,0x54]
urem16 r2, r5
# ENC: urem16 r2, r5{{.*}}encoding: [0xec,0x55]
urem16 r2, r6
# ENC: urem16 r2, r6{{.*}}encoding: [0xec,0x56]
urem16 r2, r7
# ENC: urem16 r2, r7{{.*}}encoding: [0xec,0x57]
urem16 r3, r0
# ENC: urem16 r3, r0{{.*}}encoding: [0xec,0x58]
urem16 r3, r1
# ENC: urem16 r3, r1{{.*}}encoding: [0xec,0x59]
urem16 r3, r2
# ENC: urem16 r3, r2{{.*}}encoding: [0xec,0x5a]
urem16 r3, r3
# ENC: urem16 r3, r3{{.*}}encoding: [0xec,0x5b]
urem16 r3, r4
# ENC: urem16 r3, r4{{.*}}encoding: [0xec,0x5c]
urem16 r3, r5
# ENC: urem16 r3, r5{{.*}}encoding: [0xec,0x5d]
urem16 r3, r6
# ENC: urem16 r3, r6{{.*}}encoding: [0xec,0x5e]
urem16 r3, r7
# ENC: urem16 r3, r7{{.*}}encoding: [0xec,0x5f]
urem16 r4, r0
# ENC: urem16 r4, r0{{.*}}encoding: [0xec,0x60]
urem16 r4, r1
# ENC: urem16 r4, r1{{.*}}encoding: [0xec,0x61]
urem16 r4, r2
# ENC: urem16 r4, r2{{.*}}encoding: [0xec,0x62]
urem16 r4, r3
# ENC: urem16 r4, r3{{.*}}encoding: [0xec,0x63]
urem16 r4, r4
# ENC: urem16 r4, r4{{.*}}encoding: [0xec,0x64]
urem16 r4, r5
# ENC: urem16 r4, r5{{.*}}encoding: [0xec,0x65]
urem16 r4, r6
# ENC: urem16 r4, r6{{.*}}encoding: [0xec,0x66]
urem16 r4, r7
# ENC: urem16 r4, r7{{.*}}encoding: [0xec,0x67]
urem16 r5, r0
# ENC: urem16 r5, r0{{.*}}encoding: [0xec,0x68]
urem16 r5, r1
# ENC: urem16 r5, r1{{.*}}encoding: [0xec,0x69]
urem16 r5, r2
# ENC: urem16 r5, r2{{.*}}encoding: [0xec,0x6a]
urem16 r5, r3
# ENC: urem16 r5, r3{{.*}}encoding: [0xec,0x6b]
urem16 r5, r4
# ENC: urem16 r5, r4{{.*}}encoding: [0xec,0x6c]
urem16 r5, r5
# ENC: urem16 r5, r5{{.*}}encoding: [0xec,0x6d]
urem16 r5, r6
# ENC: urem16 r5, r6{{.*}}encoding: [0xec,0x6e]
urem16 r5, r7
# ENC: urem16 r5, r7{{.*}}encoding: [0xec,0x6f]
urem16 r6, r0
# ENC: urem16 r6, r0{{.*}}encoding: [0xec,0x70]
urem16 r6, r1
# ENC: urem16 r6, r1{{.*}}encoding: [0xec,0x71]
urem16 r6, r2
# ENC: urem16 r6, r2{{.*}}encoding: [0xec,0x72]
urem16 r6, r3
# ENC: urem16 r6, r3{{.*}}encoding: [0xec,0x73]
urem16 r6, r4
# ENC: urem16 r6, r4{{.*}}encoding: [0xec,0x74]
urem16 r6, r5
# ENC: urem16 r6, r5{{.*}}encoding: [0xec,0x75]
urem16 r6, r6
# ENC: urem16 r6, r6{{.*}}encoding: [0xec,0x76]
urem16 r6, r7
# ENC: urem16 r6, r7{{.*}}encoding: [0xec,0x77]
urem16 r7, r0
# ENC: urem16 r7, r0{{.*}}encoding: [0xec,0x78]
urem16 r7, r1
# ENC: urem16 r7, r1{{.*}}encoding: [0xec,0x79]
urem16 r7, r2
# ENC: urem16 r7, r2{{.*}}encoding: [0xec,0x7a]
urem16 r7, r3
# ENC: urem16 r7, r3{{.*}}encoding: [0xec,0x7b]
urem16 r7, r4
# ENC: urem16 r7, r4{{.*}}encoding: [0xec,0x7c]
urem16 r7, r5
# ENC: urem16 r7, r5{{.*}}encoding: [0xec,0x7d]
urem16 r7, r6
# ENC: urem16 r7, r6{{.*}}encoding: [0xec,0x7e]
urem16 r7, r7
# ENC: urem16 r7, r7{{.*}}encoding: [0xec,0x7f]
sdiv16 r0, r0
# ENC: sdiv16 r0, r0{{.*}}encoding: [0xec,0x80]
sdiv16 r0, r1
# ENC: sdiv16 r0, r1{{.*}}encoding: [0xec,0x81]
sdiv16 r0, r2
# ENC: sdiv16 r0, r2{{.*}}encoding: [0xec,0x82]
sdiv16 r0, r3
# ENC: sdiv16 r0, r3{{.*}}encoding: [0xec,0x83]
sdiv16 r0, r4
# ENC: sdiv16 r0, r4{{.*}}encoding: [0xec,0x84]
sdiv16 r0, r5
# ENC: sdiv16 r0, r5{{.*}}encoding: [0xec,0x85]
sdiv16 r0, r6
# ENC: sdiv16 r0, r6{{.*}}encoding: [0xec,0x86]
sdiv16 r0, r7
# ENC: sdiv16 r0, r7{{.*}}encoding: [0xec,0x87]
sdiv16 r1, r0
# ENC: sdiv16 r1, r0{{.*}}encoding: [0xec,0x88]
sdiv16 r1, r1
# ENC: sdiv16 r1, r1{{.*}}encoding: [0xec,0x89]
sdiv16 r1, r2
# ENC: sdiv16 r1, r2{{.*}}encoding: [0xec,0x8a]
sdiv16 r1, r3
# ENC: sdiv16 r1, r3{{.*}}encoding: [0xec,0x8b]
sdiv16 r1, r4
# ENC: sdiv16 r1, r4{{.*}}encoding: [0xec,0x8c]
sdiv16 r1, r5
# ENC: sdiv16 r1, r5{{.*}}encoding: [0xec,0x8d]
sdiv16 r1, r6
# ENC: sdiv16 r1, r6{{.*}}encoding: [0xec,0x8e]
sdiv16 r1, r7
# ENC: sdiv16 r1, r7{{.*}}encoding: [0xec,0x8f]
sdiv16 r2, r0
# ENC: sdiv16 r2, r0{{.*}}encoding: [0xec,0x90]
sdiv16 r2, r1
# ENC: sdiv16 r2, r1{{.*}}encoding: [0xec,0x91]
sdiv16 r2, r2
# ENC: sdiv16 r2, r2{{.*}}encoding: [0xec,0x92]
sdiv16 r2, r3
# ENC: sdiv16 r2, r3{{.*}}encoding: [0xec,0x93]
sdiv16 r2, r4
# ENC: sdiv16 r2, r4{{.*}}encoding: [0xec,0x94]
sdiv16 r2, r5
# ENC: sdiv16 r2, r5{{.*}}encoding: [0xec,0x95]
sdiv16 r2, r6
# ENC: sdiv16 r2, r6{{.*}}encoding: [0xec,0x96]
sdiv16 r2, r7
# ENC: sdiv16 r2, r7{{.*}}encoding: [0xec,0x97]
sdiv16 r3, r0
# ENC: sdiv16 r3, r0{{.*}}encoding: [0xec,0x98]
sdiv16 r3, r1
# ENC: sdiv16 r3, r1{{.*}}encoding: [0xec,0x99]
sdiv16 r3, r2
# ENC: sdiv16 r3, r2{{.*}}encoding: [0xec,0x9a]
sdiv16 r3, r3
# ENC: sdiv16 r3, r3{{.*}}encoding: [0xec,0x9b]
sdiv16 r3, r4
# ENC: sdiv16 r3, r4{{.*}}encoding: [0xec,0x9c]
sdiv16 r3, r5
# ENC: sdiv16 r3, r5{{.*}}encoding: [0xec,0x9d]
sdiv16 r3, r6
# ENC: sdiv16 r3, r6{{.*}}encoding: [0xec,0x9e]
sdiv16 r3, r7
# ENC: sdiv16 r3, r7{{.*}}encoding: [0xec,0x9f]
sdiv16 r4, r0
# ENC: sdiv16 r4, r0{{.*}}encoding: [0xec,0xa0]
sdiv16 r4, r1
# ENC: sdiv16 r4, r1{{.*}}encoding: [0xec,0xa1]
sdiv16 r4, r2
# ENC: sdiv16 r4, r2{{.*}}encoding: [0xec,0xa2]
sdiv16 r4, r3
# ENC: sdiv16 r4, r3{{.*}}encoding: [0xec,0xa3]
sdiv16 r4, r4
# ENC: sdiv16 r4, r4{{.*}}encoding: [0xec,0xa4]
sdiv16 r4, r5
# ENC: sdiv16 r4, r5{{.*}}encoding: [0xec,0xa5]
sdiv16 r4, r6
# ENC: sdiv16 r4, r6{{.*}}encoding: [0xec,0xa6]
sdiv16 r4, r7
# ENC: sdiv16 r4, r7{{.*}}encoding: [0xec,0xa7]
sdiv16 r5, r0
# ENC: sdiv16 r5, r0{{.*}}encoding: [0xec,0xa8]
sdiv16 r5, r1
# ENC: sdiv16 r5, r1{{.*}}encoding: [0xec,0xa9]
sdiv16 r5, r2
# ENC: sdiv16 r5, r2{{.*}}encoding: [0xec,0xaa]
sdiv16 r5, r3
# ENC: sdiv16 r5, r3{{.*}}encoding: [0xec,0xab]
sdiv16 r5, r4
# ENC: sdiv16 r5, r4{{.*}}encoding: [0xec,0xac]
sdiv16 r5, r5
# ENC: sdiv16 r5, r5{{.*}}encoding: [0xec,0xad]
sdiv16 r5, r6
# ENC: sdiv16 r5, r6{{.*}}encoding: [0xec,0xae]
sdiv16 r5, r7
# ENC: sdiv16 r5, r7{{.*}}encoding: [0xec,0xaf]
sdiv16 r6, r0
# ENC: sdiv16 r6, r0{{.*}}encoding: [0xec,0xb0]
sdiv16 r6, r1
# ENC: sdiv16 r6, r1{{.*}}encoding: [0xec,0xb1]
sdiv16 r6, r2
# ENC: sdiv16 r6, r2{{.*}}encoding: [0xec,0xb2]
sdiv16 r6, r3
# ENC: sdiv16 r6, r3{{.*}}encoding: [0xec,0xb3]
sdiv16 r6, r4
# ENC: sdiv16 r6, r4{{.*}}encoding: [0xec,0xb4]
sdiv16 r6, r5
# ENC: sdiv16 r6, r5{{.*}}encoding: [0xec,0xb5]
sdiv16 r6, r6
# ENC: sdiv16 r6, r6{{.*}}encoding: [0xec,0xb6]
sdiv16 r6, r7
# ENC: sdiv16 r6, r7{{.*}}encoding: [0xec,0xb7]
sdiv16 r7, r0
# ENC: sdiv16 r7, r0{{.*}}encoding: [0xec,0xb8]
sdiv16 r7, r1
# ENC: sdiv16 r7, r1{{.*}}encoding: [0xec,0xb9]
sdiv16 r7, r2
# ENC: sdiv16 r7, r2{{.*}}encoding: [0xec,0xba]
sdiv16 r7, r3
# ENC: sdiv16 r7, r3{{.*}}encoding: [0xec,0xbb]
sdiv16 r7, r4
# ENC: sdiv16 r7, r4{{.*}}encoding: [0xec,0xbc]
sdiv16 r7, r5
# ENC: sdiv16 r7, r5{{.*}}encoding: [0xec,0xbd]
sdiv16 r7, r6
# ENC: sdiv16 r7, r6{{.*}}encoding: [0xec,0xbe]
sdiv16 r7, r7
# ENC: sdiv16 r7, r7{{.*}}encoding: [0xec,0xbf]
srem16 r0, r0
# ENC: srem16 r0, r0{{.*}}encoding: [0xec,0xc0]
srem16 r0, r1
# ENC: srem16 r0, r1{{.*}}encoding: [0xec,0xc1]
srem16 r0, r2
# ENC: srem16 r0, r2{{.*}}encoding: [0xec,0xc2]
srem16 r0, r3
# ENC: srem16 r0, r3{{.*}}encoding: [0xec,0xc3]
srem16 r0, r4
# ENC: srem16 r0, r4{{.*}}encoding: [0xec,0xc4]
srem16 r0, r5
# ENC: srem16 r0, r5{{.*}}encoding: [0xec,0xc5]
srem16 r0, r6
# ENC: srem16 r0, r6{{.*}}encoding: [0xec,0xc6]
srem16 r0, r7
# ENC: srem16 r0, r7{{.*}}encoding: [0xec,0xc7]
srem16 r1, r0
# ENC: srem16 r1, r0{{.*}}encoding: [0xec,0xc8]
srem16 r1, r1
# ENC: srem16 r1, r1{{.*}}encoding: [0xec,0xc9]
srem16 r1, r2
# ENC: srem16 r1, r2{{.*}}encoding: [0xec,0xca]
srem16 r1, r3
# ENC: srem16 r1, r3{{.*}}encoding: [0xec,0xcb]
srem16 r1, r4
# ENC: srem16 r1, r4{{.*}}encoding: [0xec,0xcc]
srem16 r1, r5
# ENC: srem16 r1, r5{{.*}}encoding: [0xec,0xcd]
srem16 r1, r6
# ENC: srem16 r1, r6{{.*}}encoding: [0xec,0xce]
srem16 r1, r7
# ENC: srem16 r1, r7{{.*}}encoding: [0xec,0xcf]
srem16 r2, r0
# ENC: srem16 r2, r0{{.*}}encoding: [0xec,0xd0]
srem16 r2, r1
# ENC: srem16 r2, r1{{.*}}encoding: [0xec,0xd1]
srem16 r2, r2
# ENC: srem16 r2, r2{{.*}}encoding: [0xec,0xd2]
srem16 r2, r3
# ENC: srem16 r2, r3{{.*}}encoding: [0xec,0xd3]
srem16 r2, r4
# ENC: srem16 r2, r4{{.*}}encoding: [0xec,0xd4]
srem16 r2, r5
# ENC: srem16 r2, r5{{.*}}encoding: [0xec,0xd5]
srem16 r2, r6
# ENC: srem16 r2, r6{{.*}}encoding: [0xec,0xd6]
srem16 r2, r7
# ENC: srem16 r2, r7{{.*}}encoding: [0xec,0xd7]
srem16 r3, r0
# ENC: srem16 r3, r0{{.*}}encoding: [0xec,0xd8]
srem16 r3, r1
# ENC: srem16 r3, r1{{.*}}encoding: [0xec,0xd9]
srem16 r3, r2
# ENC: srem16 r3, r2{{.*}}encoding: [0xec,0xda]
srem16 r3, r3
# ENC: srem16 r3, r3{{.*}}encoding: [0xec,0xdb]
srem16 r3, r4
# ENC: srem16 r3, r4{{.*}}encoding: [0xec,0xdc]
srem16 r3, r5
# ENC: srem16 r3, r5{{.*}}encoding: [0xec,0xdd]
srem16 r3, r6
# ENC: srem16 r3, r6{{.*}}encoding: [0xec,0xde]
srem16 r3, r7
# ENC: srem16 r3, r7{{.*}}encoding: [0xec,0xdf]
srem16 r4, r0
# ENC: srem16 r4, r0{{.*}}encoding: [0xec,0xe0]
srem16 r4, r1
# ENC: srem16 r4, r1{{.*}}encoding: [0xec,0xe1]
srem16 r4, r2
# ENC: srem16 r4, r2{{.*}}encoding: [0xec,0xe2]
srem16 r4, r3
# ENC: srem16 r4, r3{{.*}}encoding: [0xec,0xe3]
srem16 r4, r4
# ENC: srem16 r4, r4{{.*}}encoding: [0xec,0xe4]
srem16 r4, r5
# ENC: srem16 r4, r5{{.*}}encoding: [0xec,0xe5]
srem16 r4, r6
# ENC: srem16 r4, r6{{.*}}encoding: [0xec,0xe6]
srem16 r4, r7
# ENC: srem16 r4, r7{{.*}}encoding: [0xec,0xe7]
srem16 r5, r0
# ENC: srem16 r5, r0{{.*}}encoding: [0xec,0xe8]
srem16 r5, r1
# ENC: srem16 r5, r1{{.*}}encoding: [0xec,0xe9]
srem16 r5, r2
# ENC: srem16 r5, r2{{.*}}encoding: [0xec,0xea]
srem16 r5, r3
# ENC: srem16 r5, r3{{.*}}encoding: [0xec,0xeb]
srem16 r5, r4
# ENC: srem16 r5, r4{{.*}}encoding: [0xec,0xec]
srem16 r5, r5
# ENC: srem16 r5, r5{{.*}}encoding: [0xec,0xed]
srem16 r5, r6
# ENC: srem16 r5, r6{{.*}}encoding: [0xec,0xee]
srem16 r5, r7
# ENC: srem16 r5, r7{{.*}}encoding: [0xec,0xef]
srem16 r6, r0
# ENC: srem16 r6, r0{{.*}}encoding: [0xec,0xf0]
srem16 r6, r1
# ENC: srem16 r6, r1{{.*}}encoding: [0xec,0xf1]
srem16 r6, r2
# ENC: srem16 r6, r2{{.*}}encoding: [0xec,0xf2]
srem16 r6, r3
# ENC: srem16 r6, r3{{.*}}encoding: [0xec,0xf3]
srem16 r6, r4
# ENC: srem16 r6, r4{{.*}}encoding: [0xec,0xf4]
srem16 r6, r5
# ENC: srem16 r6, r5{{.*}}encoding: [0xec,0xf5]
srem16 r6, r6
# ENC: srem16 r6, r6{{.*}}encoding: [0xec,0xf6]
srem16 r6, r7
# ENC: srem16 r6, r7{{.*}}encoding: [0xec,0xf7]
srem16 r7, r0
# ENC: srem16 r7, r0{{.*}}encoding: [0xec,0xf8]
srem16 r7, r1
# ENC: srem16 r7, r1{{.*}}encoding: [0xec,0xf9]
srem16 r7, r2
# ENC: srem16 r7, r2{{.*}}encoding: [0xec,0xfa]
srem16 r7, r3
# ENC: srem16 r7, r3{{.*}}encoding: [0xec,0xfb]
srem16 r7, r4
# ENC: srem16 r7, r4{{.*}}encoding: [0xec,0xfc]
srem16 r7, r5
# ENC: srem16 r7, r5{{.*}}encoding: [0xec,0xfd]
srem16 r7, r6
# ENC: srem16 r7, r6{{.*}}encoding: [0xec,0xfe]
srem16 r7, r7
# ENC: srem16 r7, r7{{.*}}encoding: [0xec,0xff]
