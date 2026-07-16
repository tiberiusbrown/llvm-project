# RUN: llvm-mc -triple=avm -show-encoding %s | FileCheck %s
# RUN: llvm-mc -triple=avm -filetype=obj %s -o %t.orig.o
# RUN: llvm-objdump -d --no-show-raw-insn %t.orig.o | sed -n '/^[[:space:]]*[0-9a-f][0-9a-f]*:/s/.*:[[:space:]]*//p' > %t.reassembled.s
# RUN: llvm-mc -triple=avm -filetype=obj %t.reassembled.s -o %t.reassembled.o
# RUN: llvm-objdump -s --section=.text %t.orig.o | grep '^[[:space:]]*0000' > %t.orig.txt
# RUN: llvm-objdump -s --section=.text %t.reassembled.o | grep '^[[:space:]]*0000' > %t.reassembled.txt
# RUN: diff %t.orig.txt %t.reassembled.txt
add32 q0,q0
add32 q0,q1
add32 q0,q2
add32 q0,q3
add32 q1,q0
add32 q1,q1
add32 q1,q2
add32 q1,q3
add32 q2,q0
add32 q2,q1
add32 q2,q2
add32 q2,q3
add32 q3,q0
add32 q3,q1
add32 q3,q2
add32 q3,q3
sub32 q0,q0
sub32 q0,q1
sub32 q0,q2
sub32 q0,q3
sub32 q1,q0
sub32 q1,q1
sub32 q1,q2
sub32 q1,q3
sub32 q2,q0
sub32 q2,q1
sub32 q2,q2
sub32 q2,q3
sub32 q3,q0
sub32 q3,q1
sub32 q3,q2
sub32 q3,q3
lsr32.1 q0
lsr32.1 q1
lsr32.1 q2
lsr32.1 q3
asr32.1 q0
asr32.1 q1
asr32.1 q2
asr32.1 q3
bool r0
bool r1
bool r2
bool r3
bool r4
bool r5
bool r6
bool r7
// CHECK: add32 q0, q0{{.*}}[0xf7,0x60]
// CHECK: add32 q0, q1{{.*}}[0xf7,0x61]
// CHECK: add32 q0, q2{{.*}}[0xf7,0x62]
// CHECK: add32 q0, q3{{.*}}[0xf7,0x63]
// CHECK: add32 q1, q0{{.*}}[0xf7,0x64]
// CHECK: add32 q1, q1{{.*}}[0xf7,0x65]
// CHECK: add32 q1, q2{{.*}}[0xf7,0x66]
// CHECK: add32 q1, q3{{.*}}[0xf7,0x67]
// CHECK: add32 q2, q0{{.*}}[0xf7,0x68]
// CHECK: add32 q2, q1{{.*}}[0xf7,0x69]
// CHECK: add32 q2, q2{{.*}}[0xf7,0x6a]
// CHECK: add32 q2, q3{{.*}}[0xf7,0x6b]
// CHECK: add32 q3, q0{{.*}}[0xf7,0x6c]
// CHECK: add32 q3, q1{{.*}}[0xf7,0x6d]
// CHECK: add32 q3, q2{{.*}}[0xf7,0x6e]
// CHECK: add32 q3, q3{{.*}}[0xf7,0x6f]
// CHECK: sub32 q0, q0{{.*}}[0xf7,0x70]
// CHECK: sub32 q0, q1{{.*}}[0xf7,0x71]
// CHECK: sub32 q0, q2{{.*}}[0xf7,0x72]
// CHECK: sub32 q0, q3{{.*}}[0xf7,0x73]
// CHECK: sub32 q1, q0{{.*}}[0xf7,0x74]
// CHECK: sub32 q1, q1{{.*}}[0xf7,0x75]
// CHECK: sub32 q1, q2{{.*}}[0xf7,0x76]
// CHECK: sub32 q1, q3{{.*}}[0xf7,0x77]
// CHECK: sub32 q2, q0{{.*}}[0xf7,0x78]
// CHECK: sub32 q2, q1{{.*}}[0xf7,0x79]
// CHECK: sub32 q2, q2{{.*}}[0xf7,0x7a]
// CHECK: sub32 q2, q3{{.*}}[0xf7,0x7b]
// CHECK: sub32 q3, q0{{.*}}[0xf7,0x7c]
// CHECK: sub32 q3, q1{{.*}}[0xf7,0x7d]
// CHECK: sub32 q3, q2{{.*}}[0xf7,0x7e]
// CHECK: sub32 q3, q3{{.*}}[0xf7,0x7f]
// CHECK: lsr32.1 q0{{.*}}[0xf7,0x80]
// CHECK: lsr32.1 q1{{.*}}[0xf7,0x81]
// CHECK: lsr32.1 q2{{.*}}[0xf7,0x82]
// CHECK: lsr32.1 q3{{.*}}[0xf7,0x83]
// CHECK: asr32.1 q0{{.*}}[0xf7,0x84]
// CHECK: asr32.1 q1{{.*}}[0xf7,0x85]
// CHECK: asr32.1 q2{{.*}}[0xf7,0x86]
// CHECK: asr32.1 q3{{.*}}[0xf7,0x87]
// CHECK: bool r0{{.*}}[0xf7,0x88]
// CHECK: bool r1{{.*}}[0xf7,0x89]
// CHECK: bool r2{{.*}}[0xf7,0x8a]
// CHECK: bool r3{{.*}}[0xf7,0x8b]
// CHECK: bool r4{{.*}}[0xf7,0x8c]
// CHECK: bool r5{{.*}}[0xf7,0x8d]
// CHECK: bool r6{{.*}}[0xf7,0x8e]
// CHECK: bool r7{{.*}}[0xf7,0x8f]
