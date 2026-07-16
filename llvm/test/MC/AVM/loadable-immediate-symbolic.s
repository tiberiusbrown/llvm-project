# RUN: llvm-mc -triple=avm -show-encoding %s | FileCheck %s --check-prefix=ENC
# RUN: llvm-mc -triple=avm -filetype=obj %s -o %t.o
# RUN: llvm-readobj --relocations %t.o | FileCheck %s --check-prefix=RELOC
# RUN: llvm-objdump -d %t.o | FileCheck %s --check-prefix=DIS

.text
ldi8 c0, %hi8(symbol)
ldi8 c1, %hi8(symbol)
ldi8 c2, %hi8(symbol)
ldi8 c3, %hi8(symbol)
ldi16 c0, %lo16(symbol)
ldi16 c1, %lo16(symbol)
ldi16 c2, %lo16(symbol)
ldi16 c3, %lo16(symbol)

ldi8 r0, %hi8(symbol)
ldi8 r1, %hi8(symbol)
ldi8 r2, %hi8(symbol)
ldi8 r3, %hi8(symbol)
ldi16 r0, %lo16(symbol)
ldi16 r1, %lo16(symbol)
ldi16 r2, %lo16(symbol)
ldi16 r3, %lo16(symbol)

# ENC: ldi8{{[ \t]+}}c0, %hi8(symbol){{.*}}encoding: [0xc0,A]
# ENC-NEXT: fixup A - offset: 1, value: %hi8(symbol), kind: fixup_avm_prog_hi8
# ENC: ldi8{{[ \t]+}}c1, %hi8(symbol){{.*}}encoding: [0xc1,A]
# ENC-NEXT: fixup A - offset: 1, value: %hi8(symbol), kind: fixup_avm_prog_hi8
# ENC: ldi8{{[ \t]+}}c2, %hi8(symbol){{.*}}encoding: [0xc2,A]
# ENC-NEXT: fixup A - offset: 1, value: %hi8(symbol), kind: fixup_avm_prog_hi8
# ENC: ldi8{{[ \t]+}}c3, %hi8(symbol){{.*}}encoding: [0xc3,A]
# ENC-NEXT: fixup A - offset: 1, value: %hi8(symbol), kind: fixup_avm_prog_hi8
# ENC: ldi16{{[ \t]+}}c0, %lo16(symbol){{.*}}encoding: [0xc4,A,A]
# ENC-NEXT: fixup A - offset: 1, value: %lo16(symbol), kind: fixup_avm_prog_lo16
# ENC: ldi16{{[ \t]+}}c1, %lo16(symbol){{.*}}encoding: [0xc5,A,A]
# ENC-NEXT: fixup A - offset: 1, value: %lo16(symbol), kind: fixup_avm_prog_lo16
# ENC: ldi16{{[ \t]+}}c2, %lo16(symbol){{.*}}encoding: [0xc6,A,A]
# ENC-NEXT: fixup A - offset: 1, value: %lo16(symbol), kind: fixup_avm_prog_lo16
# ENC: ldi16{{[ \t]+}}c3, %lo16(symbol){{.*}}encoding: [0xc7,A,A]
# ENC-NEXT: fixup A - offset: 1, value: %lo16(symbol), kind: fixup_avm_prog_lo16

# ENC: ldi8{{[ \t]+}}r0, %hi8(symbol){{.*}}encoding: [0xf0,0x00,A]
# ENC-NEXT: fixup A - offset: 2, value: %hi8(symbol), kind: fixup_avm_prog_hi8
# ENC: ldi8{{[ \t]+}}r1, %hi8(symbol){{.*}}encoding: [0xf0,0x01,A]
# ENC-NEXT: fixup A - offset: 2, value: %hi8(symbol), kind: fixup_avm_prog_hi8
# ENC: ldi8{{[ \t]+}}r2, %hi8(symbol){{.*}}encoding: [0xf0,0x02,A]
# ENC-NEXT: fixup A - offset: 2, value: %hi8(symbol), kind: fixup_avm_prog_hi8
# ENC: ldi8{{[ \t]+}}r3, %hi8(symbol){{.*}}encoding: [0xf0,0x03,A]
# ENC-NEXT: fixup A - offset: 2, value: %hi8(symbol), kind: fixup_avm_prog_hi8
# ENC: ldi16{{[ \t]+}}r0, %lo16(symbol){{.*}}encoding: [0xf0,0x04,A,A]
# ENC-NEXT: fixup A - offset: 2, value: %lo16(symbol), kind: fixup_avm_prog_lo16
# ENC: ldi16{{[ \t]+}}r1, %lo16(symbol){{.*}}encoding: [0xf0,0x05,A,A]
# ENC-NEXT: fixup A - offset: 2, value: %lo16(symbol), kind: fixup_avm_prog_lo16
# ENC: ldi16{{[ \t]+}}r2, %lo16(symbol){{.*}}encoding: [0xf0,0x06,A,A]
# ENC-NEXT: fixup A - offset: 2, value: %lo16(symbol), kind: fixup_avm_prog_lo16
# ENC: ldi16{{[ \t]+}}r3, %lo16(symbol){{.*}}encoding: [0xf0,0x07,A,A]
# ENC-NEXT: fixup A - offset: 2, value: %lo16(symbol), kind: fixup_avm_prog_lo16

# RELOC: Relocations [
# RELOC: Section {{.*}} .rela.text {
# RELOC-NEXT: 0x1 R_AVM_PROG_HI8 symbol 0x0
# RELOC-NEXT: 0x3 R_AVM_PROG_HI8 symbol 0x0
# RELOC-NEXT: 0x5 R_AVM_PROG_HI8 symbol 0x0
# RELOC-NEXT: 0x7 R_AVM_PROG_HI8 symbol 0x0
# RELOC-NEXT: 0x9 R_AVM_PROG_LO16 symbol 0x0
# RELOC-NEXT: 0xC R_AVM_PROG_LO16 symbol 0x0
# RELOC-NEXT: 0xF R_AVM_PROG_LO16 symbol 0x0
# RELOC-NEXT: 0x12 R_AVM_PROG_LO16 symbol 0x0
# RELOC-NEXT: 0x16 R_AVM_PROG_HI8 symbol 0x0
# RELOC-NEXT: 0x19 R_AVM_PROG_HI8 symbol 0x0
# RELOC-NEXT: 0x1C R_AVM_PROG_HI8 symbol 0x0
# RELOC-NEXT: 0x1F R_AVM_PROG_HI8 symbol 0x0
# RELOC-NEXT: 0x22 R_AVM_PROG_LO16 symbol 0x0
# RELOC-NEXT: 0x26 R_AVM_PROG_LO16 symbol 0x0
# RELOC-NEXT: 0x2A R_AVM_PROG_LO16 symbol 0x0
# RELOC-NEXT: 0x2E R_AVM_PROG_LO16 symbol 0x0
# RELOC-NEXT: }
# RELOC-NEXT: ]

# DIS: c3 00{{ *}}ldi8{{[ \t]+}}c3, 0x0
# DIS: c6 00 00{{ *}}ldi16{{[ \t]+}}c2, 0x0
# DIS: f0 03 00{{ *}}ldi8{{[ \t]+}}r3, 0x0
# DIS: f0 06 00 00{{ *}}ldi16{{[ \t]+}}r2, 0x0
