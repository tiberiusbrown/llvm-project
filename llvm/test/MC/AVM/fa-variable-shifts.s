# RUN: llvm-mc -triple=avm --show-encoding %s | FileCheck %s --check-prefix=ENC
# RUN: llvm-mc -triple=avm --disassemble < %S/Inputs/fa-valid.txt | FileCheck %s --check-prefix=DIS
shl16v c0, c0
shl16v c0, c1
shl16v c0, c2
shl16v c0, c3
shl16v c1, c0
shl16v c1, c1
shl16v c1, c2
shl16v c1, c3
shl16v c2, c0
shl16v c2, c1
shl16v c2, c2
shl16v c2, c3
shl16v c3, c0
shl16v c3, c1
shl16v c3, c2
shl16v c3, c3
lsr16v c0, c0
lsr16v c0, c1
lsr16v c0, c2
lsr16v c0, c3
lsr16v c1, c0
lsr16v c1, c1
lsr16v c1, c2
lsr16v c1, c3
lsr16v c2, c0
lsr16v c2, c1
lsr16v c2, c2
lsr16v c2, c3
lsr16v c3, c0
lsr16v c3, c1
lsr16v c3, c2
lsr16v c3, c3
asr16v c0, c0
asr16v c0, c1
asr16v c0, c2
asr16v c0, c3
asr16v c1, c0
asr16v c1, c1
asr16v c1, c2
asr16v c1, c3
asr16v c2, c0
asr16v c2, c1
asr16v c2, c2
asr16v c2, c3
asr16v c3, c0
asr16v c3, c1
asr16v c3, c2
asr16v c3, c3
# ENC: shl16v{{[ \t]+}}c0, c0{{.*}}encoding: [0xfa,0x00]
# ENC: shl16v{{[ \t]+}}c0, c1{{.*}}encoding: [0xfa,0x01]
# ENC: shl16v{{[ \t]+}}c0, c2{{.*}}encoding: [0xfa,0x02]
# ENC: shl16v{{[ \t]+}}c0, c3{{.*}}encoding: [0xfa,0x03]
# ENC: shl16v{{[ \t]+}}c1, c0{{.*}}encoding: [0xfa,0x04]
# ENC: shl16v{{[ \t]+}}c1, c1{{.*}}encoding: [0xfa,0x05]
# ENC: shl16v{{[ \t]+}}c1, c2{{.*}}encoding: [0xfa,0x06]
# ENC: shl16v{{[ \t]+}}c1, c3{{.*}}encoding: [0xfa,0x07]
# ENC: shl16v{{[ \t]+}}c2, c0{{.*}}encoding: [0xfa,0x08]
# ENC: shl16v{{[ \t]+}}c2, c1{{.*}}encoding: [0xfa,0x09]
# ENC: shl16v{{[ \t]+}}c2, c2{{.*}}encoding: [0xfa,0x0a]
# ENC: shl16v{{[ \t]+}}c2, c3{{.*}}encoding: [0xfa,0x0b]
# ENC: shl16v{{[ \t]+}}c3, c0{{.*}}encoding: [0xfa,0x0c]
# ENC: shl16v{{[ \t]+}}c3, c1{{.*}}encoding: [0xfa,0x0d]
# ENC: shl16v{{[ \t]+}}c3, c2{{.*}}encoding: [0xfa,0x0e]
# ENC: shl16v{{[ \t]+}}c3, c3{{.*}}encoding: [0xfa,0x0f]
# ENC: lsr16v{{[ \t]+}}c0, c0{{.*}}encoding: [0xfa,0x10]
# ENC: lsr16v{{[ \t]+}}c0, c1{{.*}}encoding: [0xfa,0x11]
# ENC: lsr16v{{[ \t]+}}c0, c2{{.*}}encoding: [0xfa,0x12]
# ENC: lsr16v{{[ \t]+}}c0, c3{{.*}}encoding: [0xfa,0x13]
# ENC: lsr16v{{[ \t]+}}c1, c0{{.*}}encoding: [0xfa,0x14]
# ENC: lsr16v{{[ \t]+}}c1, c1{{.*}}encoding: [0xfa,0x15]
# ENC: lsr16v{{[ \t]+}}c1, c2{{.*}}encoding: [0xfa,0x16]
# ENC: lsr16v{{[ \t]+}}c1, c3{{.*}}encoding: [0xfa,0x17]
# ENC: lsr16v{{[ \t]+}}c2, c0{{.*}}encoding: [0xfa,0x18]
# ENC: lsr16v{{[ \t]+}}c2, c1{{.*}}encoding: [0xfa,0x19]
# ENC: lsr16v{{[ \t]+}}c2, c2{{.*}}encoding: [0xfa,0x1a]
# ENC: lsr16v{{[ \t]+}}c2, c3{{.*}}encoding: [0xfa,0x1b]
# ENC: lsr16v{{[ \t]+}}c3, c0{{.*}}encoding: [0xfa,0x1c]
# ENC: lsr16v{{[ \t]+}}c3, c1{{.*}}encoding: [0xfa,0x1d]
# ENC: lsr16v{{[ \t]+}}c3, c2{{.*}}encoding: [0xfa,0x1e]
# ENC: lsr16v{{[ \t]+}}c3, c3{{.*}}encoding: [0xfa,0x1f]
# ENC: asr16v{{[ \t]+}}c0, c0{{.*}}encoding: [0xfa,0x20]
# ENC: asr16v{{[ \t]+}}c0, c1{{.*}}encoding: [0xfa,0x21]
# ENC: asr16v{{[ \t]+}}c0, c2{{.*}}encoding: [0xfa,0x22]
# ENC: asr16v{{[ \t]+}}c0, c3{{.*}}encoding: [0xfa,0x23]
# ENC: asr16v{{[ \t]+}}c1, c0{{.*}}encoding: [0xfa,0x24]
# ENC: asr16v{{[ \t]+}}c1, c1{{.*}}encoding: [0xfa,0x25]
# ENC: asr16v{{[ \t]+}}c1, c2{{.*}}encoding: [0xfa,0x26]
# ENC: asr16v{{[ \t]+}}c1, c3{{.*}}encoding: [0xfa,0x27]
# ENC: asr16v{{[ \t]+}}c2, c0{{.*}}encoding: [0xfa,0x28]
# ENC: asr16v{{[ \t]+}}c2, c1{{.*}}encoding: [0xfa,0x29]
# ENC: asr16v{{[ \t]+}}c2, c2{{.*}}encoding: [0xfa,0x2a]
# ENC: asr16v{{[ \t]+}}c2, c3{{.*}}encoding: [0xfa,0x2b]
# ENC: asr16v{{[ \t]+}}c3, c0{{.*}}encoding: [0xfa,0x2c]
# ENC: asr16v{{[ \t]+}}c3, c1{{.*}}encoding: [0xfa,0x2d]
# ENC: asr16v{{[ \t]+}}c3, c2{{.*}}encoding: [0xfa,0x2e]
# ENC: asr16v{{[ \t]+}}c3, c3{{.*}}encoding: [0xfa,0x2f]
# DIS: shl16v{{[ \t]+}}c0, c0
# DIS: shl16v{{[ \t]+}}c0, c1
# DIS: shl16v{{[ \t]+}}c0, c2
# DIS: shl16v{{[ \t]+}}c0, c3
# DIS: shl16v{{[ \t]+}}c1, c0
# DIS: shl16v{{[ \t]+}}c1, c1
# DIS: shl16v{{[ \t]+}}c1, c2
# DIS: shl16v{{[ \t]+}}c1, c3
# DIS: shl16v{{[ \t]+}}c2, c0
# DIS: shl16v{{[ \t]+}}c2, c1
# DIS: shl16v{{[ \t]+}}c2, c2
# DIS: shl16v{{[ \t]+}}c2, c3
# DIS: shl16v{{[ \t]+}}c3, c0
# DIS: shl16v{{[ \t]+}}c3, c1
# DIS: shl16v{{[ \t]+}}c3, c2
# DIS: shl16v{{[ \t]+}}c3, c3
# DIS: lsr16v{{[ \t]+}}c0, c0
# DIS: lsr16v{{[ \t]+}}c0, c1
# DIS: lsr16v{{[ \t]+}}c0, c2
# DIS: lsr16v{{[ \t]+}}c0, c3
# DIS: lsr16v{{[ \t]+}}c1, c0
# DIS: lsr16v{{[ \t]+}}c1, c1
# DIS: lsr16v{{[ \t]+}}c1, c2
# DIS: lsr16v{{[ \t]+}}c1, c3
# DIS: lsr16v{{[ \t]+}}c2, c0
# DIS: lsr16v{{[ \t]+}}c2, c1
# DIS: lsr16v{{[ \t]+}}c2, c2
# DIS: lsr16v{{[ \t]+}}c2, c3
# DIS: lsr16v{{[ \t]+}}c3, c0
# DIS: lsr16v{{[ \t]+}}c3, c1
# DIS: lsr16v{{[ \t]+}}c3, c2
# DIS: lsr16v{{[ \t]+}}c3, c3
# DIS: asr16v{{[ \t]+}}c0, c0
# DIS: asr16v{{[ \t]+}}c0, c1
# DIS: asr16v{{[ \t]+}}c0, c2
# DIS: asr16v{{[ \t]+}}c0, c3
# DIS: asr16v{{[ \t]+}}c1, c0
# DIS: asr16v{{[ \t]+}}c1, c1
# DIS: asr16v{{[ \t]+}}c1, c2
# DIS: asr16v{{[ \t]+}}c1, c3
# DIS: asr16v{{[ \t]+}}c2, c0
# DIS: asr16v{{[ \t]+}}c2, c1
# DIS: asr16v{{[ \t]+}}c2, c2
# DIS: asr16v{{[ \t]+}}c2, c3
# DIS: asr16v{{[ \t]+}}c3, c0
# DIS: asr16v{{[ \t]+}}c3, c1
# DIS: asr16v{{[ \t]+}}c3, c2
# DIS: asr16v{{[ \t]+}}c3, c3
