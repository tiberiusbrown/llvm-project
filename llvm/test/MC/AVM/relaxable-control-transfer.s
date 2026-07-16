# RUN: llvm-mc -triple=avm -show-encoding %s | FileCheck %s --check-prefix=ENC
# RUN: llvm-mc -triple=avm -filetype=obj %s -o %t.o
# RUN: llvm-objdump -s %t.o | FileCheck %s --check-prefix=BYTES
# RUN: llvm-readobj --relocations %t.o | FileCheck %s --check-prefix=RELOC

jmp target_jmp
call target_call
breq target_eq
brne target_ne
brult target_ult
bruge target_uge
brslt target_slt
brsge target_sge

# ENC: jmp target_jmp{{.*}}encoding: [0xe2,B,B,B]
# ENC: call target_call{{.*}}encoding: [0xe3,B,B,B]
# ENC: breq target_eq{{.*}}encoding: [0xd1,0x04,0xe2,B,B,B]
# ENC: brne target_ne{{.*}}encoding: [0xd0,0x04,0xe2,B,B,B]
# ENC: brult target_ult{{.*}}encoding: [0xd8,0x04,0xe2,B,B,B]
# ENC: bruge target_uge{{.*}}encoding: [0xd2,0x04,0xe2,B,B,B]
# ENC: brslt target_slt{{.*}}encoding: [0xd9,0x04,0xe2,B,B,B]
# ENC: brsge target_sge{{.*}}encoding: [0xd3,0x04,0xe2,B,B,B]

# BYTES: 0000 e2000000 e3000000 d104e200 0000d004
# BYTES: 0010 e2000000 d804e200 0000d204 e2000000
# BYTES: 0020 d904e200 0000d304 e2000000

# RELOC: 0x0 R_AVM_RELAX - 0x0
# RELOC: 0x1 R_AVM_FAR24 target_jmp 0x0
# RELOC: 0x4 R_AVM_RELAX - 0x0
# RELOC: 0x5 R_AVM_FAR24 target_call 0x0
# RELOC: 0x8 R_AVM_RELAX - 0x0
# RELOC: 0xB R_AVM_FAR24 target_eq 0x0
# RELOC: 0xE R_AVM_RELAX - 0x0
# RELOC: 0x11 R_AVM_FAR24 target_ne 0x0
# RELOC: 0x14 R_AVM_RELAX - 0x0
# RELOC: 0x17 R_AVM_FAR24 target_ult 0x0
# RELOC: 0x1A R_AVM_RELAX - 0x0
# RELOC: 0x1D R_AVM_FAR24 target_uge 0x0
# RELOC: 0x20 R_AVM_RELAX - 0x0
# RELOC: 0x23 R_AVM_FAR24 target_slt 0x0
# RELOC: 0x26 R_AVM_RELAX - 0x0
# RELOC: 0x29 R_AVM_FAR24 target_sge 0x0
# RELOC-NOT: R_AVM_RELAX
