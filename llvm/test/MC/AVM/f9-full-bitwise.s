# RUN: llvm-mc -triple=avm --show-encoding %s | FileCheck %s
# RUN: llvm-mc -triple=avm -filetype=obj %s -o %t.orig.o
# RUN: llvm-objdump -d --no-show-raw-insn %t.orig.o | sed -n '/^[[:space:]]*[0-9a-f][0-9a-f]*:/s/.*:[[:space:]]*//p' > %t.reassembled.s
# RUN: llvm-mc -triple=avm -filetype=obj %t.reassembled.s -o %t.reassembled.o
# RUN: llvm-objdump -s --section=.text %t.orig.o | grep '^[[:space:]]*0000' > %t.orig.txt
# RUN: llvm-objdump -s --section=.text %t.reassembled.o | grep '^[[:space:]]*0000' > %t.reassembled.txt
# RUN: diff -u %t.orig.txt %t.reassembled.txt
# RUN: llvm-readobj --relocations %t.orig.o | FileCheck %s --check-prefix=NORELOC

# Exhaustive canonical source coverage: all 192 full-register pairs and all
# 48 compact spellings. Full compact/compact pairs must choose primary bytes.

# NORELOC: Relocations [
# NORELOC-NEXT: ]

and r0, r0
# CHECK: and r0, r0{{.*}}encoding: [0xf9,0x00]
and r0, r1
# CHECK: and r0, r1{{.*}}encoding: [0xf9,0x04]
and r0, r2
# CHECK: and r0, r2{{.*}}encoding: [0xf9,0x08]
and r0, r3
# CHECK: and r0, r3{{.*}}encoding: [0xf9,0x0c]
and r0, r4
# CHECK: and r0, r4{{.*}}encoding: [0xf9,0x10]
and r0, r5
# CHECK: and r0, r5{{.*}}encoding: [0xf9,0x14]
and r0, r6
# CHECK: and r0, r6{{.*}}encoding: [0xf9,0x18]
and r0, r7
# CHECK: and r0, r7{{.*}}encoding: [0xf9,0x1c]
and r1, r0
# CHECK: and r1, r0{{.*}}encoding: [0xf9,0x20]
and r1, r1
# CHECK: and r1, r1{{.*}}encoding: [0xf9,0x24]
and r1, r2
# CHECK: and r1, r2{{.*}}encoding: [0xf9,0x28]
and r1, r3
# CHECK: and r1, r3{{.*}}encoding: [0xf9,0x2c]
and r1, r4
# CHECK: and r1, r4{{.*}}encoding: [0xf9,0x30]
and r1, r5
# CHECK: and r1, r5{{.*}}encoding: [0xf9,0x34]
and r1, r6
# CHECK: and r1, r6{{.*}}encoding: [0xf9,0x38]
and r1, r7
# CHECK: and r1, r7{{.*}}encoding: [0xf9,0x3c]
and r2, r0
# CHECK: and r2, r0{{.*}}encoding: [0xf9,0x40]
and r2, r1
# CHECK: and r2, r1{{.*}}encoding: [0xf9,0x44]
and r2, r2
# CHECK: and r2, r2{{.*}}encoding: [0xf9,0x48]
and r2, r3
# CHECK: and r2, r3{{.*}}encoding: [0xf9,0x4c]
and r2, r4
# CHECK: and r2, r4{{.*}}encoding: [0xf9,0x50]
and r2, r5
# CHECK: and r2, r5{{.*}}encoding: [0xf9,0x54]
and r2, r6
# CHECK: and r2, r6{{.*}}encoding: [0xf9,0x58]
and r2, r7
# CHECK: and r2, r7{{.*}}encoding: [0xf9,0x5c]
and r3, r0
# CHECK: and r3, r0{{.*}}encoding: [0xf9,0x60]
and r3, r1
# CHECK: and r3, r1{{.*}}encoding: [0xf9,0x64]
and r3, r2
# CHECK: and r3, r2{{.*}}encoding: [0xf9,0x68]
and r3, r3
# CHECK: and r3, r3{{.*}}encoding: [0xf9,0x6c]
and r3, r4
# CHECK: and r3, r4{{.*}}encoding: [0xf9,0x70]
and r3, r5
# CHECK: and r3, r5{{.*}}encoding: [0xf9,0x74]
and r3, r6
# CHECK: and r3, r6{{.*}}encoding: [0xf9,0x78]
and r3, r7
# CHECK: and r3, r7{{.*}}encoding: [0xf9,0x7c]
and r4, r0
# CHECK: and r4, r0{{.*}}encoding: [0xf9,0x80]
and r4, r1
# CHECK: and r4, r1{{.*}}encoding: [0xf9,0x84]
and r4, r2
# CHECK: and r4, r2{{.*}}encoding: [0xf9,0x88]
and r4, r3
# CHECK: and r4, r3{{.*}}encoding: [0xf9,0x8c]
and r4, r4
# CHECK: and c0, c0{{.*}}encoding: [0x80]
and r4, r5
# CHECK: and c0, c1{{.*}}encoding: [0x81]
and r4, r6
# CHECK: and c0, c2{{.*}}encoding: [0x82]
and r4, r7
# CHECK: and c0, c3{{.*}}encoding: [0x83]
and r5, r0
# CHECK: and r5, r0{{.*}}encoding: [0xf9,0xa0]
and r5, r1
# CHECK: and r5, r1{{.*}}encoding: [0xf9,0xa4]
and r5, r2
# CHECK: and r5, r2{{.*}}encoding: [0xf9,0xa8]
and r5, r3
# CHECK: and r5, r3{{.*}}encoding: [0xf9,0xac]
and r5, r4
# CHECK: and c1, c0{{.*}}encoding: [0x84]
and r5, r5
# CHECK: and c1, c1{{.*}}encoding: [0x85]
and r5, r6
# CHECK: and c1, c2{{.*}}encoding: [0x86]
and r5, r7
# CHECK: and c1, c3{{.*}}encoding: [0x87]
and r6, r0
# CHECK: and r6, r0{{.*}}encoding: [0xf9,0xc0]
and r6, r1
# CHECK: and r6, r1{{.*}}encoding: [0xf9,0xc4]
and r6, r2
# CHECK: and r6, r2{{.*}}encoding: [0xf9,0xc8]
and r6, r3
# CHECK: and r6, r3{{.*}}encoding: [0xf9,0xcc]
and r6, r4
# CHECK: and c2, c0{{.*}}encoding: [0x88]
and r6, r5
# CHECK: and c2, c1{{.*}}encoding: [0x89]
and r6, r6
# CHECK: and c2, c2{{.*}}encoding: [0x8a]
and r6, r7
# CHECK: and c2, c3{{.*}}encoding: [0x8b]
and r7, r0
# CHECK: and r7, r0{{.*}}encoding: [0xf9,0xe0]
and r7, r1
# CHECK: and r7, r1{{.*}}encoding: [0xf9,0xe4]
and r7, r2
# CHECK: and r7, r2{{.*}}encoding: [0xf9,0xe8]
and r7, r3
# CHECK: and r7, r3{{.*}}encoding: [0xf9,0xec]
and r7, r4
# CHECK: and c3, c0{{.*}}encoding: [0x8c]
and r7, r5
# CHECK: and c3, c1{{.*}}encoding: [0x8d]
and r7, r6
# CHECK: and c3, c2{{.*}}encoding: [0x8e]
and r7, r7
# CHECK: and c3, c3{{.*}}encoding: [0x8f]
or r0, r0
# CHECK: or r0, r0{{.*}}encoding: [0xf9,0x01]
or r0, r1
# CHECK: or r0, r1{{.*}}encoding: [0xf9,0x05]
or r0, r2
# CHECK: or r0, r2{{.*}}encoding: [0xf9,0x09]
or r0, r3
# CHECK: or r0, r3{{.*}}encoding: [0xf9,0x0d]
or r0, r4
# CHECK: or r0, r4{{.*}}encoding: [0xf9,0x11]
or r0, r5
# CHECK: or r0, r5{{.*}}encoding: [0xf9,0x15]
or r0, r6
# CHECK: or r0, r6{{.*}}encoding: [0xf9,0x19]
or r0, r7
# CHECK: or r0, r7{{.*}}encoding: [0xf9,0x1d]
or r1, r0
# CHECK: or r1, r0{{.*}}encoding: [0xf9,0x21]
or r1, r1
# CHECK: or r1, r1{{.*}}encoding: [0xf9,0x25]
or r1, r2
# CHECK: or r1, r2{{.*}}encoding: [0xf9,0x29]
or r1, r3
# CHECK: or r1, r3{{.*}}encoding: [0xf9,0x2d]
or r1, r4
# CHECK: or r1, r4{{.*}}encoding: [0xf9,0x31]
or r1, r5
# CHECK: or r1, r5{{.*}}encoding: [0xf9,0x35]
or r1, r6
# CHECK: or r1, r6{{.*}}encoding: [0xf9,0x39]
or r1, r7
# CHECK: or r1, r7{{.*}}encoding: [0xf9,0x3d]
or r2, r0
# CHECK: or r2, r0{{.*}}encoding: [0xf9,0x41]
or r2, r1
# CHECK: or r2, r1{{.*}}encoding: [0xf9,0x45]
or r2, r2
# CHECK: or r2, r2{{.*}}encoding: [0xf9,0x49]
or r2, r3
# CHECK: or r2, r3{{.*}}encoding: [0xf9,0x4d]
or r2, r4
# CHECK: or r2, r4{{.*}}encoding: [0xf9,0x51]
or r2, r5
# CHECK: or r2, r5{{.*}}encoding: [0xf9,0x55]
or r2, r6
# CHECK: or r2, r6{{.*}}encoding: [0xf9,0x59]
or r2, r7
# CHECK: or r2, r7{{.*}}encoding: [0xf9,0x5d]
or r3, r0
# CHECK: or r3, r0{{.*}}encoding: [0xf9,0x61]
or r3, r1
# CHECK: or r3, r1{{.*}}encoding: [0xf9,0x65]
or r3, r2
# CHECK: or r3, r2{{.*}}encoding: [0xf9,0x69]
or r3, r3
# CHECK: or r3, r3{{.*}}encoding: [0xf9,0x6d]
or r3, r4
# CHECK: or r3, r4{{.*}}encoding: [0xf9,0x71]
or r3, r5
# CHECK: or r3, r5{{.*}}encoding: [0xf9,0x75]
or r3, r6
# CHECK: or r3, r6{{.*}}encoding: [0xf9,0x79]
or r3, r7
# CHECK: or r3, r7{{.*}}encoding: [0xf9,0x7d]
or r4, r0
# CHECK: or r4, r0{{.*}}encoding: [0xf9,0x81]
or r4, r1
# CHECK: or r4, r1{{.*}}encoding: [0xf9,0x85]
or r4, r2
# CHECK: or r4, r2{{.*}}encoding: [0xf9,0x89]
or r4, r3
# CHECK: or r4, r3{{.*}}encoding: [0xf9,0x8d]
or r4, r4
# CHECK: or c0, c0{{.*}}encoding: [0x90]
or r4, r5
# CHECK: or c0, c1{{.*}}encoding: [0x91]
or r4, r6
# CHECK: or c0, c2{{.*}}encoding: [0x92]
or r4, r7
# CHECK: or c0, c3{{.*}}encoding: [0x93]
or r5, r0
# CHECK: or r5, r0{{.*}}encoding: [0xf9,0xa1]
or r5, r1
# CHECK: or r5, r1{{.*}}encoding: [0xf9,0xa5]
or r5, r2
# CHECK: or r5, r2{{.*}}encoding: [0xf9,0xa9]
or r5, r3
# CHECK: or r5, r3{{.*}}encoding: [0xf9,0xad]
or r5, r4
# CHECK: or c1, c0{{.*}}encoding: [0x94]
or r5, r5
# CHECK: or c1, c1{{.*}}encoding: [0x95]
or r5, r6
# CHECK: or c1, c2{{.*}}encoding: [0x96]
or r5, r7
# CHECK: or c1, c3{{.*}}encoding: [0x97]
or r6, r0
# CHECK: or r6, r0{{.*}}encoding: [0xf9,0xc1]
or r6, r1
# CHECK: or r6, r1{{.*}}encoding: [0xf9,0xc5]
or r6, r2
# CHECK: or r6, r2{{.*}}encoding: [0xf9,0xc9]
or r6, r3
# CHECK: or r6, r3{{.*}}encoding: [0xf9,0xcd]
or r6, r4
# CHECK: or c2, c0{{.*}}encoding: [0x98]
or r6, r5
# CHECK: or c2, c1{{.*}}encoding: [0x99]
or r6, r6
# CHECK: or c2, c2{{.*}}encoding: [0x9a]
or r6, r7
# CHECK: or c2, c3{{.*}}encoding: [0x9b]
or r7, r0
# CHECK: or r7, r0{{.*}}encoding: [0xf9,0xe1]
or r7, r1
# CHECK: or r7, r1{{.*}}encoding: [0xf9,0xe5]
or r7, r2
# CHECK: or r7, r2{{.*}}encoding: [0xf9,0xe9]
or r7, r3
# CHECK: or r7, r3{{.*}}encoding: [0xf9,0xed]
or r7, r4
# CHECK: or c3, c0{{.*}}encoding: [0x9c]
or r7, r5
# CHECK: or c3, c1{{.*}}encoding: [0x9d]
or r7, r6
# CHECK: or c3, c2{{.*}}encoding: [0x9e]
or r7, r7
# CHECK: or c3, c3{{.*}}encoding: [0x9f]
xor r0, r0
# CHECK: xor r0, r0{{.*}}encoding: [0xf9,0x02]
xor r0, r1
# CHECK: xor r0, r1{{.*}}encoding: [0xf9,0x06]
xor r0, r2
# CHECK: xor r0, r2{{.*}}encoding: [0xf9,0x0a]
xor r0, r3
# CHECK: xor r0, r3{{.*}}encoding: [0xf9,0x0e]
xor r0, r4
# CHECK: xor r0, r4{{.*}}encoding: [0xf9,0x12]
xor r0, r5
# CHECK: xor r0, r5{{.*}}encoding: [0xf9,0x16]
xor r0, r6
# CHECK: xor r0, r6{{.*}}encoding: [0xf9,0x1a]
xor r0, r7
# CHECK: xor r0, r7{{.*}}encoding: [0xf9,0x1e]
xor r1, r0
# CHECK: xor r1, r0{{.*}}encoding: [0xf9,0x22]
xor r1, r1
# CHECK: xor r1, r1{{.*}}encoding: [0xf9,0x26]
xor r1, r2
# CHECK: xor r1, r2{{.*}}encoding: [0xf9,0x2a]
xor r1, r3
# CHECK: xor r1, r3{{.*}}encoding: [0xf9,0x2e]
xor r1, r4
# CHECK: xor r1, r4{{.*}}encoding: [0xf9,0x32]
xor r1, r5
# CHECK: xor r1, r5{{.*}}encoding: [0xf9,0x36]
xor r1, r6
# CHECK: xor r1, r6{{.*}}encoding: [0xf9,0x3a]
xor r1, r7
# CHECK: xor r1, r7{{.*}}encoding: [0xf9,0x3e]
xor r2, r0
# CHECK: xor r2, r0{{.*}}encoding: [0xf9,0x42]
xor r2, r1
# CHECK: xor r2, r1{{.*}}encoding: [0xf9,0x46]
xor r2, r2
# CHECK: xor r2, r2{{.*}}encoding: [0xf9,0x4a]
xor r2, r3
# CHECK: xor r2, r3{{.*}}encoding: [0xf9,0x4e]
xor r2, r4
# CHECK: xor r2, r4{{.*}}encoding: [0xf9,0x52]
xor r2, r5
# CHECK: xor r2, r5{{.*}}encoding: [0xf9,0x56]
xor r2, r6
# CHECK: xor r2, r6{{.*}}encoding: [0xf9,0x5a]
xor r2, r7
# CHECK: xor r2, r7{{.*}}encoding: [0xf9,0x5e]
xor r3, r0
# CHECK: xor r3, r0{{.*}}encoding: [0xf9,0x62]
xor r3, r1
# CHECK: xor r3, r1{{.*}}encoding: [0xf9,0x66]
xor r3, r2
# CHECK: xor r3, r2{{.*}}encoding: [0xf9,0x6a]
xor r3, r3
# CHECK: xor r3, r3{{.*}}encoding: [0xf9,0x6e]
xor r3, r4
# CHECK: xor r3, r4{{.*}}encoding: [0xf9,0x72]
xor r3, r5
# CHECK: xor r3, r5{{.*}}encoding: [0xf9,0x76]
xor r3, r6
# CHECK: xor r3, r6{{.*}}encoding: [0xf9,0x7a]
xor r3, r7
# CHECK: xor r3, r7{{.*}}encoding: [0xf9,0x7e]
xor r4, r0
# CHECK: xor r4, r0{{.*}}encoding: [0xf9,0x82]
xor r4, r1
# CHECK: xor r4, r1{{.*}}encoding: [0xf9,0x86]
xor r4, r2
# CHECK: xor r4, r2{{.*}}encoding: [0xf9,0x8a]
xor r4, r3
# CHECK: xor r4, r3{{.*}}encoding: [0xf9,0x8e]
xor r4, r4
# CHECK: xor c0, c0{{.*}}encoding: [0xa0]
xor r4, r5
# CHECK: xor c0, c1{{.*}}encoding: [0xa1]
xor r4, r6
# CHECK: xor c0, c2{{.*}}encoding: [0xa2]
xor r4, r7
# CHECK: xor c0, c3{{.*}}encoding: [0xa3]
xor r5, r0
# CHECK: xor r5, r0{{.*}}encoding: [0xf9,0xa2]
xor r5, r1
# CHECK: xor r5, r1{{.*}}encoding: [0xf9,0xa6]
xor r5, r2
# CHECK: xor r5, r2{{.*}}encoding: [0xf9,0xaa]
xor r5, r3
# CHECK: xor r5, r3{{.*}}encoding: [0xf9,0xae]
xor r5, r4
# CHECK: xor c1, c0{{.*}}encoding: [0xa4]
xor r5, r5
# CHECK: xor c1, c1{{.*}}encoding: [0xa5]
xor r5, r6
# CHECK: xor c1, c2{{.*}}encoding: [0xa6]
xor r5, r7
# CHECK: xor c1, c3{{.*}}encoding: [0xa7]
xor r6, r0
# CHECK: xor r6, r0{{.*}}encoding: [0xf9,0xc2]
xor r6, r1
# CHECK: xor r6, r1{{.*}}encoding: [0xf9,0xc6]
xor r6, r2
# CHECK: xor r6, r2{{.*}}encoding: [0xf9,0xca]
xor r6, r3
# CHECK: xor r6, r3{{.*}}encoding: [0xf9,0xce]
xor r6, r4
# CHECK: xor c2, c0{{.*}}encoding: [0xa8]
xor r6, r5
# CHECK: xor c2, c1{{.*}}encoding: [0xa9]
xor r6, r6
# CHECK: xor c2, c2{{.*}}encoding: [0xaa]
xor r6, r7
# CHECK: xor c2, c3{{.*}}encoding: [0xab]
xor r7, r0
# CHECK: xor r7, r0{{.*}}encoding: [0xf9,0xe2]
xor r7, r1
# CHECK: xor r7, r1{{.*}}encoding: [0xf9,0xe6]
xor r7, r2
# CHECK: xor r7, r2{{.*}}encoding: [0xf9,0xea]
xor r7, r3
# CHECK: xor r7, r3{{.*}}encoding: [0xf9,0xee]
xor r7, r4
# CHECK: xor c3, c0{{.*}}encoding: [0xac]
xor r7, r5
# CHECK: xor c3, c1{{.*}}encoding: [0xad]
xor r7, r6
# CHECK: xor c3, c2{{.*}}encoding: [0xae]
xor r7, r7
# CHECK: xor c3, c3{{.*}}encoding: [0xaf]
and c0, c0
# CHECK: and c0, c0{{.*}}encoding: [0x80]
and c0, c1
# CHECK: and c0, c1{{.*}}encoding: [0x81]
and c0, c2
# CHECK: and c0, c2{{.*}}encoding: [0x82]
and c0, c3
# CHECK: and c0, c3{{.*}}encoding: [0x83]
and c1, c0
# CHECK: and c1, c0{{.*}}encoding: [0x84]
and c1, c1
# CHECK: and c1, c1{{.*}}encoding: [0x85]
and c1, c2
# CHECK: and c1, c2{{.*}}encoding: [0x86]
and c1, c3
# CHECK: and c1, c3{{.*}}encoding: [0x87]
and c2, c0
# CHECK: and c2, c0{{.*}}encoding: [0x88]
and c2, c1
# CHECK: and c2, c1{{.*}}encoding: [0x89]
and c2, c2
# CHECK: and c2, c2{{.*}}encoding: [0x8a]
and c2, c3
# CHECK: and c2, c3{{.*}}encoding: [0x8b]
and c3, c0
# CHECK: and c3, c0{{.*}}encoding: [0x8c]
and c3, c1
# CHECK: and c3, c1{{.*}}encoding: [0x8d]
and c3, c2
# CHECK: and c3, c2{{.*}}encoding: [0x8e]
and c3, c3
# CHECK: and c3, c3{{.*}}encoding: [0x8f]
or c0, c0
# CHECK: or c0, c0{{.*}}encoding: [0x90]
or c0, c1
# CHECK: or c0, c1{{.*}}encoding: [0x91]
or c0, c2
# CHECK: or c0, c2{{.*}}encoding: [0x92]
or c0, c3
# CHECK: or c0, c3{{.*}}encoding: [0x93]
or c1, c0
# CHECK: or c1, c0{{.*}}encoding: [0x94]
or c1, c1
# CHECK: or c1, c1{{.*}}encoding: [0x95]
or c1, c2
# CHECK: or c1, c2{{.*}}encoding: [0x96]
or c1, c3
# CHECK: or c1, c3{{.*}}encoding: [0x97]
or c2, c0
# CHECK: or c2, c0{{.*}}encoding: [0x98]
or c2, c1
# CHECK: or c2, c1{{.*}}encoding: [0x99]
or c2, c2
# CHECK: or c2, c2{{.*}}encoding: [0x9a]
or c2, c3
# CHECK: or c2, c3{{.*}}encoding: [0x9b]
or c3, c0
# CHECK: or c3, c0{{.*}}encoding: [0x9c]
or c3, c1
# CHECK: or c3, c1{{.*}}encoding: [0x9d]
or c3, c2
# CHECK: or c3, c2{{.*}}encoding: [0x9e]
or c3, c3
# CHECK: or c3, c3{{.*}}encoding: [0x9f]
xor c0, c0
# CHECK: xor c0, c0{{.*}}encoding: [0xa0]
xor c0, c1
# CHECK: xor c0, c1{{.*}}encoding: [0xa1]
xor c0, c2
# CHECK: xor c0, c2{{.*}}encoding: [0xa2]
xor c0, c3
# CHECK: xor c0, c3{{.*}}encoding: [0xa3]
xor c1, c0
# CHECK: xor c1, c0{{.*}}encoding: [0xa4]
xor c1, c1
# CHECK: xor c1, c1{{.*}}encoding: [0xa5]
xor c1, c2
# CHECK: xor c1, c2{{.*}}encoding: [0xa6]
xor c1, c3
# CHECK: xor c1, c3{{.*}}encoding: [0xa7]
xor c2, c0
# CHECK: xor c2, c0{{.*}}encoding: [0xa8]
xor c2, c1
# CHECK: xor c2, c1{{.*}}encoding: [0xa9]
xor c2, c2
# CHECK: xor c2, c2{{.*}}encoding: [0xaa]
xor c2, c3
# CHECK: xor c2, c3{{.*}}encoding: [0xab]
xor c3, c0
# CHECK: xor c3, c0{{.*}}encoding: [0xac]
xor c3, c1
# CHECK: xor c3, c1{{.*}}encoding: [0xad]
xor c3, c2
# CHECK: xor c3, c2{{.*}}encoding: [0xae]
xor c3, c3
# CHECK: xor c3, c3{{.*}}encoding: [0xaf]

