# RUN: llvm-mc -triple=avm -show-encoding %s | FileCheck %s --check-prefix=ENC
# RUN: llvm-mc -triple=avm -filetype=obj %s -o %t.o
# RUN: llvm-objdump -d --no-show-raw-insn %t.o | FileCheck %s --check-prefix=DIS
# RUN: llvm-mc -triple=avm --disassemble < %S/Inputs/system-audio-service.txt \
# RUN:   | llvm-mc -triple=avm -show-encoding | FileCheck %s --check-prefix=ROUND

# A runtime audio wrapper is LDI8 r7, command; SYS 0x38; RET. The service
# instruction must consume both bytes, rather than decoding 0x38 as CMP.
ldi8 r7, 0
sys audio
ret
sys AUDIO
sys Audio
sys 0x38
sys 56

# ENC: ldi8 r7, 0{{.*}}encoding: [0xc3,0x00]
# ENC-NEXT: sys audio{{.*}}encoding: [0xd7,0x38]
# ENC-NEXT: ret{{.*}}encoding: [0xef]
# ENC-COUNT-4: sys audio{{.*}}encoding: [0xd7,0x38]

# DIS: 0: ldi8 r7, 0x0
# DIS-NEXT: 2: sys audio
# DIS-NEXT: 4: ret
# DIS-NEXT: 5: sys audio
# DIS-NEXT: 7: sys audio
# DIS-NEXT: 9: sys audio
# DIS-NEXT: b: sys audio

# ROUND: ldi8 r7, 0{{.*}}encoding: [0xc3,0x00]
# ROUND-NEXT: sys audio{{.*}}encoding: [0xd7,0x38]
# ROUND-NEXT: ret{{.*}}encoding: [0xef]
# ROUND-NEXT: sys audio{{.*}}encoding: [0xd7,0x38]
# ROUND-NEXT: sys audio{{.*}}encoding: [0xd7,0x38]
# ROUND-NEXT: ret{{.*}}encoding: [0xef]
