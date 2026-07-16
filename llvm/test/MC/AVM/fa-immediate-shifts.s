# RUN: llvm-mc -triple=avm --show-encoding %s | FileCheck %s --check-prefix=ENC
# RUN: llvm-mc -triple=avm --disassemble < %S/Inputs/fa-immediate-valid.txt | FileCheck %s --check-prefix=DIS
# RUN: llvm-mc -triple=avm -filetype=obj %s -o %t.orig.o
# RUN: llvm-objdump -d --no-show-raw-insn %t.orig.o | sed -n '/^[[:space:]]*[0-9a-f][0-9a-f]*:/s/.*:[[:space:]]*//p' > %t.reassembled.s
# RUN: llvm-mc -triple=avm -filetype=obj %t.reassembled.s -o %t.reassembled.o
# RUN: llvm-objdump -s --section=.text %t.orig.o | grep '^[[:space:]]*0000' > %t.orig.txt
# RUN: llvm-objdump -s --section=.text %t.reassembled.o | grep '^[[:space:]]*0000' > %t.reassembled.txt
# RUN: diff -u %t.orig.txt %t.reassembled.txt

lsl16i c0, 0
lsl16i c0, 1
lsl16i c0, 2
lsl16i c0, 3
lsl16i c0, 4
lsl16i c0, 5
lsl16i c0, 6
lsl16i c0, 7
lsl16i c0, 8
lsl16i c0, 9
lsl16i c0, 10
lsl16i c0, 11
lsl16i c0, 12
lsl16i c0, 13
lsl16i c0, 14
lsl16i c0, 15
lsl16i c1, 0
lsl16i c1, 1
lsl16i c1, 2
lsl16i c1, 3
lsl16i c1, 4
lsl16i c1, 5
lsl16i c1, 6
lsl16i c1, 7
lsl16i c1, 8
lsl16i c1, 9
lsl16i c1, 10
lsl16i c1, 11
lsl16i c1, 12
lsl16i c1, 13
lsl16i c1, 14
lsl16i c1, 15
lsl16i c2, 0
lsl16i c2, 1
lsl16i c2, 2
lsl16i c2, 3
lsl16i c2, 4
lsl16i c2, 5
lsl16i c2, 6
lsl16i c2, 7
lsl16i c2, 8
lsl16i c2, 9
lsl16i c2, 10
lsl16i c2, 11
lsl16i c2, 12
lsl16i c2, 13
lsl16i c2, 14
lsl16i c2, 15
lsl16i c3, 0
lsl16i c3, 1
lsl16i c3, 2
lsl16i c3, 3
lsl16i c3, 4
lsl16i c3, 5
lsl16i c3, 6
lsl16i c3, 7
lsl16i c3, 8
lsl16i c3, 9
lsl16i c3, 10
lsl16i c3, 11
lsl16i c3, 12
lsl16i c3, 13
lsl16i c3, 14
lsl16i c3, 15
lsr16i c0, 0
lsr16i c0, 1
lsr16i c0, 2
lsr16i c0, 3
lsr16i c0, 4
lsr16i c0, 5
lsr16i c0, 6
lsr16i c0, 7
lsr16i c0, 8
lsr16i c0, 9
lsr16i c0, 10
lsr16i c0, 11
lsr16i c0, 12
lsr16i c0, 13
lsr16i c0, 14
lsr16i c0, 15
lsr16i c1, 0
lsr16i c1, 1
lsr16i c1, 2
lsr16i c1, 3
lsr16i c1, 4
lsr16i c1, 5
lsr16i c1, 6
lsr16i c1, 7
lsr16i c1, 8
lsr16i c1, 9
lsr16i c1, 10
lsr16i c1, 11
lsr16i c1, 12
lsr16i c1, 13
lsr16i c1, 14
lsr16i c1, 15
lsr16i c2, 0
lsr16i c2, 1
lsr16i c2, 2
lsr16i c2, 3
lsr16i c2, 4
lsr16i c2, 5
lsr16i c2, 6
lsr16i c2, 7
lsr16i c2, 8
lsr16i c2, 9
lsr16i c2, 10
lsr16i c2, 11
lsr16i c2, 12
lsr16i c2, 13
lsr16i c2, 14
lsr16i c2, 15
lsr16i c3, 0
lsr16i c3, 1
lsr16i c3, 2
lsr16i c3, 3
lsr16i c3, 4
lsr16i c3, 5
lsr16i c3, 6
lsr16i c3, 7
lsr16i c3, 8
lsr16i c3, 9
lsr16i c3, 10
lsr16i c3, 11
lsr16i c3, 12
lsr16i c3, 13
lsr16i c3, 14
lsr16i c3, 15
asr16i c0, 0
asr16i c0, 1
asr16i c0, 2
asr16i c0, 3
asr16i c0, 4
asr16i c0, 5
asr16i c0, 6
asr16i c0, 7
asr16i c0, 8
asr16i c0, 9
asr16i c0, 10
asr16i c0, 11
asr16i c0, 12
asr16i c0, 13
asr16i c0, 14
asr16i c0, 15
asr16i c1, 0
asr16i c1, 1
asr16i c1, 2
asr16i c1, 3
asr16i c1, 4
asr16i c1, 5
asr16i c1, 6
asr16i c1, 7
asr16i c1, 8
asr16i c1, 9
asr16i c1, 10
asr16i c1, 11
asr16i c1, 12
asr16i c1, 13
asr16i c1, 14
asr16i c1, 15
asr16i c2, 0
asr16i c2, 1
asr16i c2, 2
asr16i c2, 3
asr16i c2, 4
asr16i c2, 5
asr16i c2, 6
asr16i c2, 7
asr16i c2, 8
asr16i c2, 9
asr16i c2, 10
asr16i c2, 11
asr16i c2, 12
asr16i c2, 13
asr16i c2, 14
asr16i c2, 15
asr16i c3, 0
asr16i c3, 1
asr16i c3, 2
asr16i c3, 3
asr16i c3, 4
asr16i c3, 5
asr16i c3, 6
asr16i c3, 7
asr16i c3, 8
asr16i c3, 9
asr16i c3, 10
asr16i c3, 11
asr16i c3, 12
asr16i c3, 13
asr16i c3, 14
asr16i c3, 15

# ENC: lsl16i{{[ \t]+}}c0, 0{{.*}}encoding: [0xfa,0x30]
# ENC: lsl16i{{[ \t]+}}c0, 1{{.*}}encoding: [0xfa,0x31]
# ENC: lsl16i{{[ \t]+}}c0, 2{{.*}}encoding: [0xfa,0x32]
# ENC: lsl16i{{[ \t]+}}c0, 3{{.*}}encoding: [0xfa,0x33]
# ENC: lsl16i{{[ \t]+}}c0, 4{{.*}}encoding: [0xfa,0x34]
# ENC: lsl16i{{[ \t]+}}c0, 5{{.*}}encoding: [0xfa,0x35]
# ENC: lsl16i{{[ \t]+}}c0, 6{{.*}}encoding: [0xfa,0x36]
# ENC: lsl16i{{[ \t]+}}c0, 7{{.*}}encoding: [0xfa,0x37]
# ENC: lsl16i{{[ \t]+}}c0, 8{{.*}}encoding: [0xfa,0x38]
# ENC: lsl16i{{[ \t]+}}c0, 9{{.*}}encoding: [0xfa,0x39]
# ENC: lsl16i{{[ \t]+}}c0, 10{{.*}}encoding: [0xfa,0x3a]
# ENC: lsl16i{{[ \t]+}}c0, 11{{.*}}encoding: [0xfa,0x3b]
# ENC: lsl16i{{[ \t]+}}c0, 12{{.*}}encoding: [0xfa,0x3c]
# ENC: lsl16i{{[ \t]+}}c0, 13{{.*}}encoding: [0xfa,0x3d]
# ENC: lsl16i{{[ \t]+}}c0, 14{{.*}}encoding: [0xfa,0x3e]
# ENC: lsl16i{{[ \t]+}}c0, 15{{.*}}encoding: [0xfa,0x3f]
# ENC: lsl16i{{[ \t]+}}c1, 0{{.*}}encoding: [0xfa,0x40]
# ENC: lsl16i{{[ \t]+}}c1, 1{{.*}}encoding: [0xfa,0x41]
# ENC: lsl16i{{[ \t]+}}c1, 2{{.*}}encoding: [0xfa,0x42]
# ENC: lsl16i{{[ \t]+}}c1, 3{{.*}}encoding: [0xfa,0x43]
# ENC: lsl16i{{[ \t]+}}c1, 4{{.*}}encoding: [0xfa,0x44]
# ENC: lsl16i{{[ \t]+}}c1, 5{{.*}}encoding: [0xfa,0x45]
# ENC: lsl16i{{[ \t]+}}c1, 6{{.*}}encoding: [0xfa,0x46]
# ENC: lsl16i{{[ \t]+}}c1, 7{{.*}}encoding: [0xfa,0x47]
# ENC: lsl16i{{[ \t]+}}c1, 8{{.*}}encoding: [0xfa,0x48]
# ENC: lsl16i{{[ \t]+}}c1, 9{{.*}}encoding: [0xfa,0x49]
# ENC: lsl16i{{[ \t]+}}c1, 10{{.*}}encoding: [0xfa,0x4a]
# ENC: lsl16i{{[ \t]+}}c1, 11{{.*}}encoding: [0xfa,0x4b]
# ENC: lsl16i{{[ \t]+}}c1, 12{{.*}}encoding: [0xfa,0x4c]
# ENC: lsl16i{{[ \t]+}}c1, 13{{.*}}encoding: [0xfa,0x4d]
# ENC: lsl16i{{[ \t]+}}c1, 14{{.*}}encoding: [0xfa,0x4e]
# ENC: lsl16i{{[ \t]+}}c1, 15{{.*}}encoding: [0xfa,0x4f]
# ENC: lsl16i{{[ \t]+}}c2, 0{{.*}}encoding: [0xfa,0x50]
# ENC: lsl16i{{[ \t]+}}c2, 1{{.*}}encoding: [0xfa,0x51]
# ENC: lsl16i{{[ \t]+}}c2, 2{{.*}}encoding: [0xfa,0x52]
# ENC: lsl16i{{[ \t]+}}c2, 3{{.*}}encoding: [0xfa,0x53]
# ENC: lsl16i{{[ \t]+}}c2, 4{{.*}}encoding: [0xfa,0x54]
# ENC: lsl16i{{[ \t]+}}c2, 5{{.*}}encoding: [0xfa,0x55]
# ENC: lsl16i{{[ \t]+}}c2, 6{{.*}}encoding: [0xfa,0x56]
# ENC: lsl16i{{[ \t]+}}c2, 7{{.*}}encoding: [0xfa,0x57]
# ENC: lsl16i{{[ \t]+}}c2, 8{{.*}}encoding: [0xfa,0x58]
# ENC: lsl16i{{[ \t]+}}c2, 9{{.*}}encoding: [0xfa,0x59]
# ENC: lsl16i{{[ \t]+}}c2, 10{{.*}}encoding: [0xfa,0x5a]
# ENC: lsl16i{{[ \t]+}}c2, 11{{.*}}encoding: [0xfa,0x5b]
# ENC: lsl16i{{[ \t]+}}c2, 12{{.*}}encoding: [0xfa,0x5c]
# ENC: lsl16i{{[ \t]+}}c2, 13{{.*}}encoding: [0xfa,0x5d]
# ENC: lsl16i{{[ \t]+}}c2, 14{{.*}}encoding: [0xfa,0x5e]
# ENC: lsl16i{{[ \t]+}}c2, 15{{.*}}encoding: [0xfa,0x5f]
# ENC: lsl16i{{[ \t]+}}c3, 0{{.*}}encoding: [0xfa,0x60]
# ENC: lsl16i{{[ \t]+}}c3, 1{{.*}}encoding: [0xfa,0x61]
# ENC: lsl16i{{[ \t]+}}c3, 2{{.*}}encoding: [0xfa,0x62]
# ENC: lsl16i{{[ \t]+}}c3, 3{{.*}}encoding: [0xfa,0x63]
# ENC: lsl16i{{[ \t]+}}c3, 4{{.*}}encoding: [0xfa,0x64]
# ENC: lsl16i{{[ \t]+}}c3, 5{{.*}}encoding: [0xfa,0x65]
# ENC: lsl16i{{[ \t]+}}c3, 6{{.*}}encoding: [0xfa,0x66]
# ENC: lsl16i{{[ \t]+}}c3, 7{{.*}}encoding: [0xfa,0x67]
# ENC: lsl16i{{[ \t]+}}c3, 8{{.*}}encoding: [0xfa,0x68]
# ENC: lsl16i{{[ \t]+}}c3, 9{{.*}}encoding: [0xfa,0x69]
# ENC: lsl16i{{[ \t]+}}c3, 10{{.*}}encoding: [0xfa,0x6a]
# ENC: lsl16i{{[ \t]+}}c3, 11{{.*}}encoding: [0xfa,0x6b]
# ENC: lsl16i{{[ \t]+}}c3, 12{{.*}}encoding: [0xfa,0x6c]
# ENC: lsl16i{{[ \t]+}}c3, 13{{.*}}encoding: [0xfa,0x6d]
# ENC: lsl16i{{[ \t]+}}c3, 14{{.*}}encoding: [0xfa,0x6e]
# ENC: lsl16i{{[ \t]+}}c3, 15{{.*}}encoding: [0xfa,0x6f]
# ENC: lsr16i{{[ \t]+}}c0, 0{{.*}}encoding: [0xfa,0x70]
# ENC: lsr16i{{[ \t]+}}c0, 1{{.*}}encoding: [0xfa,0x71]
# ENC: lsr16i{{[ \t]+}}c0, 2{{.*}}encoding: [0xfa,0x72]
# ENC: lsr16i{{[ \t]+}}c0, 3{{.*}}encoding: [0xfa,0x73]
# ENC: lsr16i{{[ \t]+}}c0, 4{{.*}}encoding: [0xfa,0x74]
# ENC: lsr16i{{[ \t]+}}c0, 5{{.*}}encoding: [0xfa,0x75]
# ENC: lsr16i{{[ \t]+}}c0, 6{{.*}}encoding: [0xfa,0x76]
# ENC: lsr16i{{[ \t]+}}c0, 7{{.*}}encoding: [0xfa,0x77]
# ENC: lsr16i{{[ \t]+}}c0, 8{{.*}}encoding: [0xfa,0x78]
# ENC: lsr16i{{[ \t]+}}c0, 9{{.*}}encoding: [0xfa,0x79]
# ENC: lsr16i{{[ \t]+}}c0, 10{{.*}}encoding: [0xfa,0x7a]
# ENC: lsr16i{{[ \t]+}}c0, 11{{.*}}encoding: [0xfa,0x7b]
# ENC: lsr16i{{[ \t]+}}c0, 12{{.*}}encoding: [0xfa,0x7c]
# ENC: lsr16i{{[ \t]+}}c0, 13{{.*}}encoding: [0xfa,0x7d]
# ENC: lsr16i{{[ \t]+}}c0, 14{{.*}}encoding: [0xfa,0x7e]
# ENC: lsr16i{{[ \t]+}}c0, 15{{.*}}encoding: [0xfa,0x7f]
# ENC: lsr16i{{[ \t]+}}c1, 0{{.*}}encoding: [0xfa,0x80]
# ENC: lsr16i{{[ \t]+}}c1, 1{{.*}}encoding: [0xfa,0x81]
# ENC: lsr16i{{[ \t]+}}c1, 2{{.*}}encoding: [0xfa,0x82]
# ENC: lsr16i{{[ \t]+}}c1, 3{{.*}}encoding: [0xfa,0x83]
# ENC: lsr16i{{[ \t]+}}c1, 4{{.*}}encoding: [0xfa,0x84]
# ENC: lsr16i{{[ \t]+}}c1, 5{{.*}}encoding: [0xfa,0x85]
# ENC: lsr16i{{[ \t]+}}c1, 6{{.*}}encoding: [0xfa,0x86]
# ENC: lsr16i{{[ \t]+}}c1, 7{{.*}}encoding: [0xfa,0x87]
# ENC: lsr16i{{[ \t]+}}c1, 8{{.*}}encoding: [0xfa,0x88]
# ENC: lsr16i{{[ \t]+}}c1, 9{{.*}}encoding: [0xfa,0x89]
# ENC: lsr16i{{[ \t]+}}c1, 10{{.*}}encoding: [0xfa,0x8a]
# ENC: lsr16i{{[ \t]+}}c1, 11{{.*}}encoding: [0xfa,0x8b]
# ENC: lsr16i{{[ \t]+}}c1, 12{{.*}}encoding: [0xfa,0x8c]
# ENC: lsr16i{{[ \t]+}}c1, 13{{.*}}encoding: [0xfa,0x8d]
# ENC: lsr16i{{[ \t]+}}c1, 14{{.*}}encoding: [0xfa,0x8e]
# ENC: lsr16i{{[ \t]+}}c1, 15{{.*}}encoding: [0xfa,0x8f]
# ENC: lsr16i{{[ \t]+}}c2, 0{{.*}}encoding: [0xfa,0x90]
# ENC: lsr16i{{[ \t]+}}c2, 1{{.*}}encoding: [0xfa,0x91]
# ENC: lsr16i{{[ \t]+}}c2, 2{{.*}}encoding: [0xfa,0x92]
# ENC: lsr16i{{[ \t]+}}c2, 3{{.*}}encoding: [0xfa,0x93]
# ENC: lsr16i{{[ \t]+}}c2, 4{{.*}}encoding: [0xfa,0x94]
# ENC: lsr16i{{[ \t]+}}c2, 5{{.*}}encoding: [0xfa,0x95]
# ENC: lsr16i{{[ \t]+}}c2, 6{{.*}}encoding: [0xfa,0x96]
# ENC: lsr16i{{[ \t]+}}c2, 7{{.*}}encoding: [0xfa,0x97]
# ENC: lsr16i{{[ \t]+}}c2, 8{{.*}}encoding: [0xfa,0x98]
# ENC: lsr16i{{[ \t]+}}c2, 9{{.*}}encoding: [0xfa,0x99]
# ENC: lsr16i{{[ \t]+}}c2, 10{{.*}}encoding: [0xfa,0x9a]
# ENC: lsr16i{{[ \t]+}}c2, 11{{.*}}encoding: [0xfa,0x9b]
# ENC: lsr16i{{[ \t]+}}c2, 12{{.*}}encoding: [0xfa,0x9c]
# ENC: lsr16i{{[ \t]+}}c2, 13{{.*}}encoding: [0xfa,0x9d]
# ENC: lsr16i{{[ \t]+}}c2, 14{{.*}}encoding: [0xfa,0x9e]
# ENC: lsr16i{{[ \t]+}}c2, 15{{.*}}encoding: [0xfa,0x9f]
# ENC: lsr16i{{[ \t]+}}c3, 0{{.*}}encoding: [0xfa,0xa0]
# ENC: lsr16i{{[ \t]+}}c3, 1{{.*}}encoding: [0xfa,0xa1]
# ENC: lsr16i{{[ \t]+}}c3, 2{{.*}}encoding: [0xfa,0xa2]
# ENC: lsr16i{{[ \t]+}}c3, 3{{.*}}encoding: [0xfa,0xa3]
# ENC: lsr16i{{[ \t]+}}c3, 4{{.*}}encoding: [0xfa,0xa4]
# ENC: lsr16i{{[ \t]+}}c3, 5{{.*}}encoding: [0xfa,0xa5]
# ENC: lsr16i{{[ \t]+}}c3, 6{{.*}}encoding: [0xfa,0xa6]
# ENC: lsr16i{{[ \t]+}}c3, 7{{.*}}encoding: [0xfa,0xa7]
# ENC: lsr16i{{[ \t]+}}c3, 8{{.*}}encoding: [0xfa,0xa8]
# ENC: lsr16i{{[ \t]+}}c3, 9{{.*}}encoding: [0xfa,0xa9]
# ENC: lsr16i{{[ \t]+}}c3, 10{{.*}}encoding: [0xfa,0xaa]
# ENC: lsr16i{{[ \t]+}}c3, 11{{.*}}encoding: [0xfa,0xab]
# ENC: lsr16i{{[ \t]+}}c3, 12{{.*}}encoding: [0xfa,0xac]
# ENC: lsr16i{{[ \t]+}}c3, 13{{.*}}encoding: [0xfa,0xad]
# ENC: lsr16i{{[ \t]+}}c3, 14{{.*}}encoding: [0xfa,0xae]
# ENC: lsr16i{{[ \t]+}}c3, 15{{.*}}encoding: [0xfa,0xaf]
# ENC: asr16i{{[ \t]+}}c0, 0{{.*}}encoding: [0xfa,0xb0]
# ENC: asr16i{{[ \t]+}}c0, 1{{.*}}encoding: [0xfa,0xb1]
# ENC: asr16i{{[ \t]+}}c0, 2{{.*}}encoding: [0xfa,0xb2]
# ENC: asr16i{{[ \t]+}}c0, 3{{.*}}encoding: [0xfa,0xb3]
# ENC: asr16i{{[ \t]+}}c0, 4{{.*}}encoding: [0xfa,0xb4]
# ENC: asr16i{{[ \t]+}}c0, 5{{.*}}encoding: [0xfa,0xb5]
# ENC: asr16i{{[ \t]+}}c0, 6{{.*}}encoding: [0xfa,0xb6]
# ENC: asr16i{{[ \t]+}}c0, 7{{.*}}encoding: [0xfa,0xb7]
# ENC: asr16i{{[ \t]+}}c0, 8{{.*}}encoding: [0xfa,0xb8]
# ENC: asr16i{{[ \t]+}}c0, 9{{.*}}encoding: [0xfa,0xb9]
# ENC: asr16i{{[ \t]+}}c0, 10{{.*}}encoding: [0xfa,0xba]
# ENC: asr16i{{[ \t]+}}c0, 11{{.*}}encoding: [0xfa,0xbb]
# ENC: asr16i{{[ \t]+}}c0, 12{{.*}}encoding: [0xfa,0xbc]
# ENC: asr16i{{[ \t]+}}c0, 13{{.*}}encoding: [0xfa,0xbd]
# ENC: asr16i{{[ \t]+}}c0, 14{{.*}}encoding: [0xfa,0xbe]
# ENC: asr16i{{[ \t]+}}c0, 15{{.*}}encoding: [0xfa,0xbf]
# ENC: asr16i{{[ \t]+}}c1, 0{{.*}}encoding: [0xfa,0xc0]
# ENC: asr16i{{[ \t]+}}c1, 1{{.*}}encoding: [0xfa,0xc1]
# ENC: asr16i{{[ \t]+}}c1, 2{{.*}}encoding: [0xfa,0xc2]
# ENC: asr16i{{[ \t]+}}c1, 3{{.*}}encoding: [0xfa,0xc3]
# ENC: asr16i{{[ \t]+}}c1, 4{{.*}}encoding: [0xfa,0xc4]
# ENC: asr16i{{[ \t]+}}c1, 5{{.*}}encoding: [0xfa,0xc5]
# ENC: asr16i{{[ \t]+}}c1, 6{{.*}}encoding: [0xfa,0xc6]
# ENC: asr16i{{[ \t]+}}c1, 7{{.*}}encoding: [0xfa,0xc7]
# ENC: asr16i{{[ \t]+}}c1, 8{{.*}}encoding: [0xfa,0xc8]
# ENC: asr16i{{[ \t]+}}c1, 9{{.*}}encoding: [0xfa,0xc9]
# ENC: asr16i{{[ \t]+}}c1, 10{{.*}}encoding: [0xfa,0xca]
# ENC: asr16i{{[ \t]+}}c1, 11{{.*}}encoding: [0xfa,0xcb]
# ENC: asr16i{{[ \t]+}}c1, 12{{.*}}encoding: [0xfa,0xcc]
# ENC: asr16i{{[ \t]+}}c1, 13{{.*}}encoding: [0xfa,0xcd]
# ENC: asr16i{{[ \t]+}}c1, 14{{.*}}encoding: [0xfa,0xce]
# ENC: asr16i{{[ \t]+}}c1, 15{{.*}}encoding: [0xfa,0xcf]
# ENC: asr16i{{[ \t]+}}c2, 0{{.*}}encoding: [0xfa,0xd0]
# ENC: asr16i{{[ \t]+}}c2, 1{{.*}}encoding: [0xfa,0xd1]
# ENC: asr16i{{[ \t]+}}c2, 2{{.*}}encoding: [0xfa,0xd2]
# ENC: asr16i{{[ \t]+}}c2, 3{{.*}}encoding: [0xfa,0xd3]
# ENC: asr16i{{[ \t]+}}c2, 4{{.*}}encoding: [0xfa,0xd4]
# ENC: asr16i{{[ \t]+}}c2, 5{{.*}}encoding: [0xfa,0xd5]
# ENC: asr16i{{[ \t]+}}c2, 6{{.*}}encoding: [0xfa,0xd6]
# ENC: asr16i{{[ \t]+}}c2, 7{{.*}}encoding: [0xfa,0xd7]
# ENC: asr16i{{[ \t]+}}c2, 8{{.*}}encoding: [0xfa,0xd8]
# ENC: asr16i{{[ \t]+}}c2, 9{{.*}}encoding: [0xfa,0xd9]
# ENC: asr16i{{[ \t]+}}c2, 10{{.*}}encoding: [0xfa,0xda]
# ENC: asr16i{{[ \t]+}}c2, 11{{.*}}encoding: [0xfa,0xdb]
# ENC: asr16i{{[ \t]+}}c2, 12{{.*}}encoding: [0xfa,0xdc]
# ENC: asr16i{{[ \t]+}}c2, 13{{.*}}encoding: [0xfa,0xdd]
# ENC: asr16i{{[ \t]+}}c2, 14{{.*}}encoding: [0xfa,0xde]
# ENC: asr16i{{[ \t]+}}c2, 15{{.*}}encoding: [0xfa,0xdf]
# ENC: asr16i{{[ \t]+}}c3, 0{{.*}}encoding: [0xfa,0xe0]
# ENC: asr16i{{[ \t]+}}c3, 1{{.*}}encoding: [0xfa,0xe1]
# ENC: asr16i{{[ \t]+}}c3, 2{{.*}}encoding: [0xfa,0xe2]
# ENC: asr16i{{[ \t]+}}c3, 3{{.*}}encoding: [0xfa,0xe3]
# ENC: asr16i{{[ \t]+}}c3, 4{{.*}}encoding: [0xfa,0xe4]
# ENC: asr16i{{[ \t]+}}c3, 5{{.*}}encoding: [0xfa,0xe5]
# ENC: asr16i{{[ \t]+}}c3, 6{{.*}}encoding: [0xfa,0xe6]
# ENC: asr16i{{[ \t]+}}c3, 7{{.*}}encoding: [0xfa,0xe7]
# ENC: asr16i{{[ \t]+}}c3, 8{{.*}}encoding: [0xfa,0xe8]
# ENC: asr16i{{[ \t]+}}c3, 9{{.*}}encoding: [0xfa,0xe9]
# ENC: asr16i{{[ \t]+}}c3, 10{{.*}}encoding: [0xfa,0xea]
# ENC: asr16i{{[ \t]+}}c3, 11{{.*}}encoding: [0xfa,0xeb]
# ENC: asr16i{{[ \t]+}}c3, 12{{.*}}encoding: [0xfa,0xec]
# ENC: asr16i{{[ \t]+}}c3, 13{{.*}}encoding: [0xfa,0xed]
# ENC: asr16i{{[ \t]+}}c3, 14{{.*}}encoding: [0xfa,0xee]
# ENC: asr16i{{[ \t]+}}c3, 15{{.*}}encoding: [0xfa,0xef]

# DIS: lsl16i{{[ \t]+}}c0, 0
# DIS: lsl16i{{[ \t]+}}c0, 1
# DIS: lsl16i{{[ \t]+}}c0, 2
# DIS: lsl16i{{[ \t]+}}c0, 3
# DIS: lsl16i{{[ \t]+}}c0, 4
# DIS: lsl16i{{[ \t]+}}c0, 5
# DIS: lsl16i{{[ \t]+}}c0, 6
# DIS: lsl16i{{[ \t]+}}c0, 7
# DIS: lsl16i{{[ \t]+}}c0, 8
# DIS: lsl16i{{[ \t]+}}c0, 9
# DIS: lsl16i{{[ \t]+}}c0, 10
# DIS: lsl16i{{[ \t]+}}c0, 11
# DIS: lsl16i{{[ \t]+}}c0, 12
# DIS: lsl16i{{[ \t]+}}c0, 13
# DIS: lsl16i{{[ \t]+}}c0, 14
# DIS: lsl16i{{[ \t]+}}c0, 15
# DIS: lsl16i{{[ \t]+}}c1, 0
# DIS: lsl16i{{[ \t]+}}c1, 1
# DIS: lsl16i{{[ \t]+}}c1, 2
# DIS: lsl16i{{[ \t]+}}c1, 3
# DIS: lsl16i{{[ \t]+}}c1, 4
# DIS: lsl16i{{[ \t]+}}c1, 5
# DIS: lsl16i{{[ \t]+}}c1, 6
# DIS: lsl16i{{[ \t]+}}c1, 7
# DIS: lsl16i{{[ \t]+}}c1, 8
# DIS: lsl16i{{[ \t]+}}c1, 9
# DIS: lsl16i{{[ \t]+}}c1, 10
# DIS: lsl16i{{[ \t]+}}c1, 11
# DIS: lsl16i{{[ \t]+}}c1, 12
# DIS: lsl16i{{[ \t]+}}c1, 13
# DIS: lsl16i{{[ \t]+}}c1, 14
# DIS: lsl16i{{[ \t]+}}c1, 15
# DIS: lsl16i{{[ \t]+}}c2, 0
# DIS: lsl16i{{[ \t]+}}c2, 1
# DIS: lsl16i{{[ \t]+}}c2, 2
# DIS: lsl16i{{[ \t]+}}c2, 3
# DIS: lsl16i{{[ \t]+}}c2, 4
# DIS: lsl16i{{[ \t]+}}c2, 5
# DIS: lsl16i{{[ \t]+}}c2, 6
# DIS: lsl16i{{[ \t]+}}c2, 7
# DIS: lsl16i{{[ \t]+}}c2, 8
# DIS: lsl16i{{[ \t]+}}c2, 9
# DIS: lsl16i{{[ \t]+}}c2, 10
# DIS: lsl16i{{[ \t]+}}c2, 11
# DIS: lsl16i{{[ \t]+}}c2, 12
# DIS: lsl16i{{[ \t]+}}c2, 13
# DIS: lsl16i{{[ \t]+}}c2, 14
# DIS: lsl16i{{[ \t]+}}c2, 15
# DIS: lsl16i{{[ \t]+}}c3, 0
# DIS: lsl16i{{[ \t]+}}c3, 1
# DIS: lsl16i{{[ \t]+}}c3, 2
# DIS: lsl16i{{[ \t]+}}c3, 3
# DIS: lsl16i{{[ \t]+}}c3, 4
# DIS: lsl16i{{[ \t]+}}c3, 5
# DIS: lsl16i{{[ \t]+}}c3, 6
# DIS: lsl16i{{[ \t]+}}c3, 7
# DIS: lsl16i{{[ \t]+}}c3, 8
# DIS: lsl16i{{[ \t]+}}c3, 9
# DIS: lsl16i{{[ \t]+}}c3, 10
# DIS: lsl16i{{[ \t]+}}c3, 11
# DIS: lsl16i{{[ \t]+}}c3, 12
# DIS: lsl16i{{[ \t]+}}c3, 13
# DIS: lsl16i{{[ \t]+}}c3, 14
# DIS: lsl16i{{[ \t]+}}c3, 15
# DIS: lsr16i{{[ \t]+}}c0, 0
# DIS: lsr16i{{[ \t]+}}c0, 1
# DIS: lsr16i{{[ \t]+}}c0, 2
# DIS: lsr16i{{[ \t]+}}c0, 3
# DIS: lsr16i{{[ \t]+}}c0, 4
# DIS: lsr16i{{[ \t]+}}c0, 5
# DIS: lsr16i{{[ \t]+}}c0, 6
# DIS: lsr16i{{[ \t]+}}c0, 7
# DIS: lsr16i{{[ \t]+}}c0, 8
# DIS: lsr16i{{[ \t]+}}c0, 9
# DIS: lsr16i{{[ \t]+}}c0, 10
# DIS: lsr16i{{[ \t]+}}c0, 11
# DIS: lsr16i{{[ \t]+}}c0, 12
# DIS: lsr16i{{[ \t]+}}c0, 13
# DIS: lsr16i{{[ \t]+}}c0, 14
# DIS: lsr16i{{[ \t]+}}c0, 15
# DIS: lsr16i{{[ \t]+}}c1, 0
# DIS: lsr16i{{[ \t]+}}c1, 1
# DIS: lsr16i{{[ \t]+}}c1, 2
# DIS: lsr16i{{[ \t]+}}c1, 3
# DIS: lsr16i{{[ \t]+}}c1, 4
# DIS: lsr16i{{[ \t]+}}c1, 5
# DIS: lsr16i{{[ \t]+}}c1, 6
# DIS: lsr16i{{[ \t]+}}c1, 7
# DIS: lsr16i{{[ \t]+}}c1, 8
# DIS: lsr16i{{[ \t]+}}c1, 9
# DIS: lsr16i{{[ \t]+}}c1, 10
# DIS: lsr16i{{[ \t]+}}c1, 11
# DIS: lsr16i{{[ \t]+}}c1, 12
# DIS: lsr16i{{[ \t]+}}c1, 13
# DIS: lsr16i{{[ \t]+}}c1, 14
# DIS: lsr16i{{[ \t]+}}c1, 15
# DIS: lsr16i{{[ \t]+}}c2, 0
# DIS: lsr16i{{[ \t]+}}c2, 1
# DIS: lsr16i{{[ \t]+}}c2, 2
# DIS: lsr16i{{[ \t]+}}c2, 3
# DIS: lsr16i{{[ \t]+}}c2, 4
# DIS: lsr16i{{[ \t]+}}c2, 5
# DIS: lsr16i{{[ \t]+}}c2, 6
# DIS: lsr16i{{[ \t]+}}c2, 7
# DIS: lsr16i{{[ \t]+}}c2, 8
# DIS: lsr16i{{[ \t]+}}c2, 9
# DIS: lsr16i{{[ \t]+}}c2, 10
# DIS: lsr16i{{[ \t]+}}c2, 11
# DIS: lsr16i{{[ \t]+}}c2, 12
# DIS: lsr16i{{[ \t]+}}c2, 13
# DIS: lsr16i{{[ \t]+}}c2, 14
# DIS: lsr16i{{[ \t]+}}c2, 15
# DIS: lsr16i{{[ \t]+}}c3, 0
# DIS: lsr16i{{[ \t]+}}c3, 1
# DIS: lsr16i{{[ \t]+}}c3, 2
# DIS: lsr16i{{[ \t]+}}c3, 3
# DIS: lsr16i{{[ \t]+}}c3, 4
# DIS: lsr16i{{[ \t]+}}c3, 5
# DIS: lsr16i{{[ \t]+}}c3, 6
# DIS: lsr16i{{[ \t]+}}c3, 7
# DIS: lsr16i{{[ \t]+}}c3, 8
# DIS: lsr16i{{[ \t]+}}c3, 9
# DIS: lsr16i{{[ \t]+}}c3, 10
# DIS: lsr16i{{[ \t]+}}c3, 11
# DIS: lsr16i{{[ \t]+}}c3, 12
# DIS: lsr16i{{[ \t]+}}c3, 13
# DIS: lsr16i{{[ \t]+}}c3, 14
# DIS: lsr16i{{[ \t]+}}c3, 15
# DIS: asr16i{{[ \t]+}}c0, 0
# DIS: asr16i{{[ \t]+}}c0, 1
# DIS: asr16i{{[ \t]+}}c0, 2
# DIS: asr16i{{[ \t]+}}c0, 3
# DIS: asr16i{{[ \t]+}}c0, 4
# DIS: asr16i{{[ \t]+}}c0, 5
# DIS: asr16i{{[ \t]+}}c0, 6
# DIS: asr16i{{[ \t]+}}c0, 7
# DIS: asr16i{{[ \t]+}}c0, 8
# DIS: asr16i{{[ \t]+}}c0, 9
# DIS: asr16i{{[ \t]+}}c0, 10
# DIS: asr16i{{[ \t]+}}c0, 11
# DIS: asr16i{{[ \t]+}}c0, 12
# DIS: asr16i{{[ \t]+}}c0, 13
# DIS: asr16i{{[ \t]+}}c0, 14
# DIS: asr16i{{[ \t]+}}c0, 15
# DIS: asr16i{{[ \t]+}}c1, 0
# DIS: asr16i{{[ \t]+}}c1, 1
# DIS: asr16i{{[ \t]+}}c1, 2
# DIS: asr16i{{[ \t]+}}c1, 3
# DIS: asr16i{{[ \t]+}}c1, 4
# DIS: asr16i{{[ \t]+}}c1, 5
# DIS: asr16i{{[ \t]+}}c1, 6
# DIS: asr16i{{[ \t]+}}c1, 7
# DIS: asr16i{{[ \t]+}}c1, 8
# DIS: asr16i{{[ \t]+}}c1, 9
# DIS: asr16i{{[ \t]+}}c1, 10
# DIS: asr16i{{[ \t]+}}c1, 11
# DIS: asr16i{{[ \t]+}}c1, 12
# DIS: asr16i{{[ \t]+}}c1, 13
# DIS: asr16i{{[ \t]+}}c1, 14
# DIS: asr16i{{[ \t]+}}c1, 15
# DIS: asr16i{{[ \t]+}}c2, 0
# DIS: asr16i{{[ \t]+}}c2, 1
# DIS: asr16i{{[ \t]+}}c2, 2
# DIS: asr16i{{[ \t]+}}c2, 3
# DIS: asr16i{{[ \t]+}}c2, 4
# DIS: asr16i{{[ \t]+}}c2, 5
# DIS: asr16i{{[ \t]+}}c2, 6
# DIS: asr16i{{[ \t]+}}c2, 7
# DIS: asr16i{{[ \t]+}}c2, 8
# DIS: asr16i{{[ \t]+}}c2, 9
# DIS: asr16i{{[ \t]+}}c2, 10
# DIS: asr16i{{[ \t]+}}c2, 11
# DIS: asr16i{{[ \t]+}}c2, 12
# DIS: asr16i{{[ \t]+}}c2, 13
# DIS: asr16i{{[ \t]+}}c2, 14
# DIS: asr16i{{[ \t]+}}c2, 15
# DIS: asr16i{{[ \t]+}}c3, 0
# DIS: asr16i{{[ \t]+}}c3, 1
# DIS: asr16i{{[ \t]+}}c3, 2
# DIS: asr16i{{[ \t]+}}c3, 3
# DIS: asr16i{{[ \t]+}}c3, 4
# DIS: asr16i{{[ \t]+}}c3, 5
# DIS: asr16i{{[ \t]+}}c3, 6
# DIS: asr16i{{[ \t]+}}c3, 7
# DIS: asr16i{{[ \t]+}}c3, 8
# DIS: asr16i{{[ \t]+}}c3, 9
# DIS: asr16i{{[ \t]+}}c3, 10
# DIS: asr16i{{[ \t]+}}c3, 11
# DIS: asr16i{{[ \t]+}}c3, 12
# DIS: asr16i{{[ \t]+}}c3, 13
# DIS: asr16i{{[ \t]+}}c3, 14
# DIS: asr16i{{[ \t]+}}c3, 15

