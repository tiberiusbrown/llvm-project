# RUN: split-file %s %t
# RUN: llvm-mc -triple=avm -filetype=obj %t/defs.s -o %t/defs.o
# RUN: llvm-mc -triple=avm -filetype=obj %t/refs.s -o %t/refs.o
# RUN: ld.lld -T %t/layout.ld %t/refs.o %t/defs.o -o %t/a.out
# RUN: llvm-readobj --file-headers --sections %t/a.out | FileCheck %s --check-prefix=HDR
# RUN: llvm-objdump -s -j .text -j .data %t/a.out | FileCheck %s --check-prefix=DATA
# RUN: llvm-mc -triple=avm -filetype=obj %t/pcrel8-good.s -o %t/pcrel8-good.o
# RUN: ld.lld -Ttext=0x10000 --defsym=plus8=0x10081 --defsym=minus8=0xff84 %t/pcrel8-good.o -o %t/pcrel8.out
# RUN: llvm-objdump -s -j .text %t/pcrel8.out | FileCheck %s --check-prefix=PCREL8
# RUN: llvm-mc -triple=avm -filetype=obj %t/pcrel8-bad.s -o %t/pcrel8-bad.o
# RUN: not ld.lld -Ttext=0x10000 --defsym=too_far8=0x10082 %t/pcrel8-bad.o -o %t/bad8.out 2>&1 | FileCheck %s --check-prefix=ERR8
# RUN: llvm-mc -triple=avm -filetype=obj %t/pcrel16-good.s -o %t/pcrel16-good.o
# RUN: ld.lld -Ttext=0x10000 --defsym=plus16=0x18002 %t/pcrel16-good.o -o %t/pcrel16.out
# RUN: llvm-objdump -s -j .text %t/pcrel16.out | FileCheck %s --check-prefix=PCREL16
# RUN: llvm-mc -triple=avm -filetype=obj %t/pcrel16-bad.s -o %t/pcrel16-bad.o
# RUN: not ld.lld -Ttext=0x10000 --defsym=too_far16=0x18003 %t/pcrel16-bad.o -o %t/bad16.out 2>&1 | FileCheck %s --check-prefix=ERR16
# RUN: llvm-mc -triple=avm -filetype=obj %t/relax.s -o %t/relax.o
# RUN: ld.lld -T %t/layout.ld %t/relax.o %t/defs.o -o %t/relax.out
# RUN: llvm-readobj --sections %t/relax.out | FileCheck %s --check-prefix=RELAX
# RUN: llvm-objdump -s -j .text %t/relax.out | FileCheck %s --check-prefix=RELAX-BYTES
# RUN: ld.lld --no-relax -T %t/layout.ld %t/relax.o %t/defs.o -o %t/relax-no.out
# RUN: llvm-readobj --sections %t/relax-no.out | FileCheck %s --check-prefix=RELAX-NO
# RUN: llvm-objdump -s -j .text %t/relax-no.out | FileCheck %s --check-prefix=RELAX-NO-BYTES
# RUN: ld.lld --emit-relocs -T %t/layout.ld %t/relax.o %t/defs.o -o %t/relax-emit.out
# RUN: llvm-readobj -r %t/relax-emit.out | FileCheck %s --check-prefix=RELAX-EMIT
# RUN: llvm-objdump -s -j .text %t/relax-emit.out | FileCheck %s --check-prefix=RELAX-EMIT-BYTES
# RUN: ld.lld -r %t/relax.o -o %t/relax-r.o
# RUN: llvm-readobj -r %t/relax-r.o | FileCheck %s --check-prefix=RELAX-R
# RUN: ld.lld -T %t/layout.ld %t/relax-r.o %t/defs.o -o %t/relax-r.out
# RUN: llvm-objdump -s -j .text %t/relax-r.out | FileCheck %s --check-prefix=RELAX-R-BYTES

# HDR: Format: elf32-avm
# HDR: Arch: avm
# HDR: Flags [ (0x1)
# HDR: Name: .text
# HDR: Name: .data
# DATA: Contents of section .text:
# DATA: 123450 05020002 693412c4 6434c012 e2643412
# DATA: 123460 e36b3412 00
# DATA: Contents of section .data:
# DATA: 0200 020200
# PCREL8: Contents of section .text:
# PCREL8: 10000 d47fd580
# PCREL16: Contents of section .text:
# PCREL16: 10000 e0ff7f
# ERR8: relocation R_AVM_PCREL8 out of range
# ERR16: relocation R_AVM_PCREL16 out of range
# RELAX: Name: .text
# RELAX: Size: 18
# RELAX-BYTES: Contents of section .text:
# RELAX-BYTES: 123450 d40fd50d d00bd109 d207d805 d303d901
# RELAX-BYTES: 123460 0000
# RELAX-NO: Name: .text
# RELAX-NO: Size: 46
# RELAX-NO-BYTES: Contents of section .text:
# RELAX-NO-BYTES: 123450 e27d3412 e37d3412 d104e27d 3412d004
# RELAX-NO-BYTES: 123460 e27d3412 d804e27d 3412d204 e27d3412
# RELAX-NO-BYTES: 123470 d904e27d 3412d304 e27d3412 0000
# RELAX-EMIT: R_AVM_RELAX
# RELAX-EMIT: R_AVM_FAR24 relax_target
# RELAX-EMIT-BYTES: Contents of section .text:
# RELAX-EMIT-BYTES: 123450 e27d3412 e37d3412 d104e27d 3412d004
# RELAX-EMIT-BYTES: 123460 e27d3412 d804e27d 3412d204 e27d3412
# RELAX-EMIT-BYTES: 123470 d904e27d 3412d304 e27d3412 0000
# RELAX-R: R_AVM_RELAX
# RELAX-R: R_AVM_FAR24 relax_target
# RELAX-R-BYTES: Contents of section .text:
# RELAX-R-BYTES: 123450 d40fd50d d00bd109 d207d805 d303d901
# RELAX-R-BYTES: 123460 0000

#--- defs.s
.section .text
.globl program_symbol
program_symbol:
  .byte 0
.globl relax_target
relax_target:
  .byte 0

.section .data
.globl data_symbol
data_symbol:
  .byte 0

#--- refs.s
.section .text
.short data_symbol+3
.short data_symbol-2
.progptr program_symbol+5
ldi16 c0, %lo16(program_symbol)
ldi8 c1, %hi8(program_symbol)
jmpf program_symbol
callf program_symbol+7

.section .data
.short data_symbol

#--- layout.ld
SECTIONS {
  .text 0x123450 : { *(.text) }
  .data 0x200 : { *(.data) }
}

#--- pcrel8-good.s
.text
jmp8 plus8
call8 minus8

#--- pcrel8-bad.s
.text
jmp8 too_far8

#--- pcrel16-good.s
.text
jmp16 plus16

#--- pcrel16-bad.s
.text
jmp16 too_far16

#--- relax.s
.text
jmp relax_target
call relax_target
br.eq relax_target
br.ne relax_target
br.ult relax_target
br.uge relax_target
br.slt relax_target
br.sge relax_target
