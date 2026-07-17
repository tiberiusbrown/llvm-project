# RUN: llvm-mc -triple=avm -show-encoding %s | FileCheck %s --check-prefix=ENCODING
# RUN: llvm-mc -triple=avm -filetype=obj %s -o %t.orig.o
# RUN: llvm-objdump -d --no-show-raw-insn %t.orig.o | FileCheck %s --check-prefix=DISASM
# RUN: llvm-objdump -d --no-show-raw-insn %t.orig.o | sed -n '/^[[:space:]]*[0-9a-f][0-9a-f]*:/s/.*:[[:space:]]*//p' > %t.reassembled.s
# RUN: llvm-mc -triple=avm -filetype=obj %t.reassembled.s -o %t.reassembled.o
# RUN: llvm-objdump -s --section=.text %t.orig.o | grep '^[[:space:]]*0000' > %t.orig.txt
# RUN: llvm-objdump -s --section=.text %t.reassembled.o | grep '^[[:space:]]*0000' > %t.reassembled.txt
# RUN: diff -u %t.orig.txt %t.reassembled.txt

.globl _start
_start:
  ldi8 c0, 'P'
  sys 0
  sys debug_putc
  sys DEBUG_PUTC
  sys Debug_Putc
  sys dEbUg_pUtC
  sys 1
  sys debug_break
  sys DEBUG_BREAK
  sys Debug_Break
  sys dEbUg_bReAk
  sys 2
  sys 3

# ENCODING: ldi8{{.*}}encoding: [0xc0,0x50]
# ENCODING-COUNT-5: sys{{.*}}encoding: [0xd7,0x00]
# ENCODING-COUNT-5: sys{{.*}}encoding: [0xd7,0x01]
# ENCODING: sys{{.*}}encoding: [0xd7,0x02]
# ENCODING: sys{{.*}}encoding: [0xd7,0x03]

# DISASM: ldi8 r4, 0x50
# DISASM-COUNT-5: sys debug_putc
# DISASM-COUNT-5: sys debug_break
# DISASM: sys millis
# DISASM: sys millis32
