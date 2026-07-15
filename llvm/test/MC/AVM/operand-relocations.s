# RUN: llvm-mc -triple=avm-unknown-arduboyfx -show-encoding %s | FileCheck %s --check-prefix=FIXUP
# RUN: llvm-mc -triple=avm-unknown-arduboyfx -filetype=obj %s -o %t.o
# RUN: llvm-readobj --relocations %t.o | FileCheck %s --check-prefix=RELOC
# RUN: llvm-objdump -dr %t.o | FileCheck %s --check-prefix=DIS

.text
.globl external_prog
.globl external_data
local_prog:
  ldi8  r1, prog_hi8(local_prog + 4)
  ldi8  c1, prog_hi8(external_prog + 4)
  cmpi8 r2, prog_hi8(external_prog)

  ldi16  r0, prog_lo16(local_prog + 4)
  addi16 r1, prog_lo16(external_prog + 4)
  subi16 r2, prog_lo16(external_prog)
  andi16 r3, prog_lo16(external_prog)
  ori16  r4, prog_lo16(external_prog)
  xori16 r5, prog_lo16(external_prog)
  cmpi16 r6, prog_lo16(external_prog)

  ldi16  r0, external_data + 2
  addi16 r1, external_data
  subi16 r2, external_data
  andi16 r3, external_data
  ori16  r4, external_data
  xori16 r5, external_data
  cmpi16 r6, external_data

  ldpbi prog_hi8(local_prog)
  ldm8  r0, local_data + 1
  stm8  external_data + 2, r1
  ldm16 r2, external_data
  stm16 local_data, r3

  breq external_prog + 2
  brne external_prog
  brult external_prog
  bruge external_prog
  brslt external_prog
  brsge external_prog
  brule external_prog
  brugt external_prog
  jmp external_prog
  call external_prog

  jmp16 local_prog
  call16 external_prog + 2
  jmpf local_prog
  callf external_prog + 4

.data
local_data:
  .byte 0

# FIXUP: ldi8 r1, prog_hi8(local_prog+4){{.*}}encoding: [0xe0,0x89,A]
# FIXUP: fixup A - offset: 2, value: prog_hi8(local_prog+4), kind: fixup_avm_prog_hi8
# FIXUP: ldi8 c1, prog_hi8(external_prog+4){{.*}}encoding: [0xf1,A]
# FIXUP: fixup A - offset: 1, value: prog_hi8(external_prog+4), kind: fixup_avm_prog_hi8
# FIXUP: cmpi8 r2, prog_hi8(external_prog){{.*}}encoding: [0xe0,0xc2,A]
# FIXUP: ldi16 r0, prog_lo16(local_prog+4){{.*}}encoding: [0xe0,0x80,A,A]
# FIXUP: fixup A - offset: 2, value: prog_lo16(local_prog+4), kind: fixup_avm_prog_lo16
# FIXUP: addi16 r1, prog_lo16(external_prog+4){{.*}}encoding: [0xe0,0x91,A,A]
# FIXUP: ldi16 r0, external_data+2{{.*}}encoding: [0xe0,0x80,A,A]
# FIXUP: fixup A - offset: 2, value: external_data+2, kind: fixup_avm_data16
# FIXUP: ldm8 r0, local_data+1{{.*}}encoding: [0xfd,0x30,A,A]
# FIXUP: fixup A - offset: 2, value: local_data+1, kind: fixup_avm_data16
# FIXUP: breq external_prog+2{{.*}}encoding: [0xf5,A]
# FIXUP: fixup A - offset: 1, value: external_prog+2-1, kind: fixup_avm_pcrel8
# FIXUP: jmp16 local_prog{{.*}}encoding: [0xea,A,A]
# FIXUP: fixup A - offset: 1, value: local_prog-2, kind: fixup_avm_pcrel16
# FIXUP: callf external_prog+4
# FIXUP: fixup A - offset: 1, value: external_prog+4, kind: fixup_avm_far24
# FIXUP: fixup B - offset: 1, value: external_prog+4, kind: fixup_avm_relax

# RELOC-DAG: R_AVM_PROG_HI8 .text 0x4
# RELOC-DAG: R_AVM_PROG_HI8 external_prog 0x4
# RELOC-DAG: R_AVM_PROG_LO16 .text 0x4
# RELOC-DAG: R_AVM_DATA16 external_data 0x2
# RELOC-DAG: R_AVM_DATA16 .data 0x1
# RELOC-DAG: R_AVM_PCREL8 external_prog 0x1
# RELOC-DAG: R_AVM_PCREL16 .text
# RELOC-DAG: R_AVM_FAR24 .text 0x0
# RELOC-DAG: R_AVM_FAR24 external_prog 0x4
# RELOC-DAG: R_AVM_RELAX .text 0x0
# RELOC-DAG: R_AVM_RELAX external_prog 0x4

# DIS: R_AVM_PROG_HI8 .text+0x4
# DIS: R_AVM_PROG_LO16 .text+0x4
# DIS: R_AVM_DATA16 external_data+0x2
# DIS: R_AVM_PCREL8 external_prog+0x1
# DIS: R_AVM_PCREL16 .text
# DIS: R_AVM_FAR24 external_prog+0x4
# DIS: R_AVM_RELAX external_prog+0x4
