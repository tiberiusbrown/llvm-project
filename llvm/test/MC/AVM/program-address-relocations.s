# RUN: llvm-mc -triple=avm-unknown-arduboyfx -show-encoding %s | FileCheck %s --check-prefix=ENC
# RUN: llvm-mc -triple=avm-unknown-arduboyfx -filetype=obj %s -o %t.o
# RUN: llvm-readobj --relocations %t.o | FileCheck %s --check-prefix=RELOC
# RUN: llvm-objdump -dr %t.o | FileCheck %s --check-prefix=DIS

.section .text,"ax",@progbits
.globl _start
_start:
  ldpbi prog_hi8(ldp_test_data)
  ldi16 r6, prog_lo16(ldp_test_data)
  ldp8 r0, [pb:r6]

  ldpbi prog_hi8(external_data + 3)
  ldi16 r7, prog_lo16(external_data + 3)

  ldpbi prog_hi8(0x123456)
  ldi16 r6, prog_lo16(0x123456)

.section .rodata,"a",@progbits
ldp_test_data:
  .byte 0xd3, 0x5a, 0xc7, 0x80

# ENC: ldpbi prog_hi8(ldp_test_data){{.*}}encoding: [0xe8,A]
# ENC: fixup A - offset: 1, value: prog_hi8(ldp_test_data), kind: fixup_avm_prog_hi8
# ENC: ldi16 r6, prog_lo16(ldp_test_data){{.*}}encoding: [0xe0,0x86,A,A]
# ENC: fixup A - offset: 2, value: prog_lo16(ldp_test_data), kind: fixup_avm_prog_lo16
# ENC: ldpbi prog_hi8(external_data+3){{.*}}encoding: [0xe8,A]
# ENC: ldi16 r7, prog_lo16(external_data+3){{.*}}encoding: [0xe0,0x87,A,A]
# ENC: ldpbi prog_hi8(1193046){{.*}}encoding: [0xe8,0x12]
# ENC: ldi16 r6, prog_lo16(1193046){{.*}}encoding: [0xe0,0x86,0x56,0x34]

# RELOC: R_AVM_PROG_HI8 .rodata 0x0
# RELOC: R_AVM_PROG_LO16 .rodata 0x0
# RELOC: R_AVM_PROG_HI8 external_data 0x3
# RELOC: R_AVM_PROG_LO16 external_data 0x3

# DIS: R_AVM_PROG_HI8 .rodata
# DIS: R_AVM_PROG_LO16 .rodata
# DIS: R_AVM_PROG_HI8 external_data+0x3
# DIS: R_AVM_PROG_LO16 external_data+0x3
