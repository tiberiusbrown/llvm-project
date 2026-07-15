# RUN: llvm-mc -triple=avm --show-encoding %s | FileCheck %s
# RUN: llvm-mc -triple=avm -filetype=obj %s -o %t.o
# RUN: llvm-objdump -d --no-show-raw-insn %t.o | FileCheck %s --check-prefix=ROUNDTRIP
# RUN: llvm-readobj --relocations %t.o | FileCheck %s --check-prefix=NORELOC

lsl16.1 r0
lsl16.1 r1
lsl16.1 r2
lsl16.1 r3
lsl16.1 r4
lsl16.1 r5
lsl16.1 r6
lsl16.1 r7
lsr16.1 r0
lsr16.1 r1
lsr16.1 r2
lsr16.1 r3
lsr16.1 r4
lsr16.1 r5
lsr16.1 r6
lsr16.1 r7
asr16.1 r0
asr16.1 r1
asr16.1 r2
asr16.1 r3
asr16.1 r4
asr16.1 r5
asr16.1 r6
asr16.1 r7
not16 r0
not16 r1
not16 r2
not16 r3
not16 r4
not16 r5
not16 r6
not16 r7
tst8 r0
tst8 r1
tst8 r2
tst8 r3
tst8 r4
tst8 r5
tst8 r6
tst8 r7
inc16 r0
inc16 r1
inc16 r2
inc16 r3
inc16 r4
inc16 r5
inc16 r6
inc16 r7
dec16 r0
dec16 r1
dec16 r2
dec16 r3
dec16 r4
dec16 r5
dec16 r6
dec16 r7

# CHECK: encoding: [0xf4,0x80]
# CHECK: encoding: [0xf4,0x81]
# CHECK: encoding: [0xf4,0x82]
# CHECK: encoding: [0xf4,0x83]
# CHECK: encoding: [0xf4,0x84]
# CHECK: encoding: [0xf4,0x85]
# CHECK: encoding: [0xf4,0x86]
# CHECK: encoding: [0xf4,0x87]
# CHECK: encoding: [0xf4,0x88]
# CHECK: encoding: [0xf4,0x89]
# CHECK: encoding: [0xf4,0x8a]
# CHECK: encoding: [0xf4,0x8b]
# CHECK: encoding: [0xf4,0x8c]
# CHECK: encoding: [0xf4,0x8d]
# CHECK: encoding: [0xf4,0x8e]
# CHECK: encoding: [0xf4,0x8f]
# CHECK: encoding: [0xf4,0x90]
# CHECK: encoding: [0xf4,0x91]
# CHECK: encoding: [0xf4,0x92]
# CHECK: encoding: [0xf4,0x93]
# CHECK: encoding: [0xf4,0x94]
# CHECK: encoding: [0xf4,0x95]
# CHECK: encoding: [0xf4,0x96]
# CHECK: encoding: [0xf4,0x97]
# CHECK: encoding: [0xf4,0x98]
# CHECK: encoding: [0xf4,0x99]
# CHECK: encoding: [0xf4,0x9a]
# CHECK: encoding: [0xf4,0x9b]
# CHECK: encoding: [0xf4,0x9c]
# CHECK: encoding: [0xf4,0x9d]
# CHECK: encoding: [0xf4,0x9e]
# CHECK: encoding: [0xf4,0x9f]
# CHECK: encoding: [0xf4,0xa0]
# CHECK: encoding: [0xf4,0xa1]
# CHECK: encoding: [0xf4,0xa2]
# CHECK: encoding: [0xf4,0xa3]
# CHECK: encoding: [0xf4,0xa4]
# CHECK: encoding: [0xf4,0xa5]
# CHECK: encoding: [0xf4,0xa6]
# CHECK: encoding: [0xf4,0xa7]
# CHECK: encoding: [0xf4,0xa8]
# CHECK: encoding: [0xf4,0xa9]
# CHECK: encoding: [0xf4,0xaa]
# CHECK: encoding: [0xf4,0xab]
# CHECK: encoding: [0xf4,0xac]
# CHECK: encoding: [0xf4,0xad]
# CHECK: encoding: [0xf4,0xae]
# CHECK: encoding: [0xf4,0xaf]
# CHECK: encoding: [0xf4,0xb0]
# CHECK: encoding: [0xf4,0xb1]
# CHECK: encoding: [0xf4,0xb2]
# CHECK: encoding: [0xf4,0xb3]
# CHECK: encoding: [0xf4,0xb4]
# CHECK: encoding: [0xf4,0xb5]
# CHECK: encoding: [0xf4,0xb6]
# CHECK: encoding: [0xf4,0xb7]

# ROUNDTRIP-COUNT-8: lsl16.1
# ROUNDTRIP-COUNT-8: lsr16.1
# ROUNDTRIP-COUNT-8: asr16.1
# ROUNDTRIP-COUNT-8: not16
# ROUNDTRIP-COUNT-8: tst8
# ROUNDTRIP-COUNT-8: inc16
# ROUNDTRIP-COUNT-8: dec16
# NORELOC: Relocations [
# NORELOC-NEXT: ]
