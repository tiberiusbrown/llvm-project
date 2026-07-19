# RUN: llvm-mc -triple=avm -filetype=obj %s -o %t.o
# RUN: ld.lld -e _start -Ttext=0x10000 --defsym=symbol=0x1234ab %t.o -o %t
# RUN: llvm-objdump -s -j .text %t | FileCheck %s --check-prefix=BYTES
# RUN: llvm-objdump -d %t | FileCheck %s --check-prefix=DIS

.text
.globl _start
_start:
  ldi8 c1, %hi8(symbol)
  ldi16 c3, %lo16(symbol)
  ldi8 r2, %hi8(symbol)
  ldi16 r3, %lo16(symbol)

# BYTES: Contents of section .text:
# BYTES: 10000 c112c7ab 34f00212 f007ab34

# DIS: ldi8{{[ \t]+}}r5, 0x12
# DIS: ldi16{{[ \t]+}}r7, 0x34ab
# DIS: ldi8{{[ \t]+}}r2, 0x12
# DIS: ldi16{{[ \t]+}}r3, 0x34ab
