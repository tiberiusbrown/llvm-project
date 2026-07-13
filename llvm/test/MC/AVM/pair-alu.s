# RUN: llvm-mc -triple=avm-unknown-arduboyfx -show-encoding %s | FileCheck %s --check-prefix=ENC
# RUN: llvm-mc -triple=avm-unknown-arduboyfx -filetype=obj %s -o %t.o
# RUN: llvm-objdump -d --no-show-raw-insn %t.o | FileCheck %s --check-prefix=DIS

mov32 q0, q0
add32 q0, q1
sub32 q1, q2
and32 q2, q3
or32 q3, q0
xor32 q0, q3
cmp32 q1, q0
shl32v q2, q1
lsr32v q3, q2
asr32v q3, q3

# ENC: mov32 q0, q0{{.*}}encoding: [0xe1,0x00]
# ENC: add32 q0, q1{{.*}}encoding: [0xe1,0x11]
# ENC: sub32 q1, q2{{.*}}encoding: [0xe1,0x26]
# ENC: and32 q2, q3{{.*}}encoding: [0xe1,0x3b]
# ENC: or32 q3, q0{{.*}}encoding: [0xe1,0x4c]
# ENC: xor32 q0, q3{{.*}}encoding: [0xe1,0x53]
# ENC: cmp32 q1, q0{{.*}}encoding: [0xe1,0x64]
# ENC: shl32v q2, q1{{.*}}encoding: [0xe1,0x79]
# ENC: lsr32v q3, q2{{.*}}encoding: [0xe1,0x8e]
# ENC: asr32v q3, q3{{.*}}encoding: [0xe1,0x9f]

# DIS: mov32 q0, q0
# DIS: add32 q0, q1
# DIS: sub32 q1, q2
# DIS: and32 q2, q3
# DIS: or32 q3, q0
# DIS: xor32 q0, q3
# DIS: cmp32 q1, q0
# DIS: shl32v q2, q1
# DIS: lsr32v q3, q2
# DIS: asr32v q3, q3
