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
  sys millis
  sys 3
  sys millis32
  sys 4
  sys sinf
  sys 5
  sys cosf
  sys 6
  sys atan2f
  sys 7
  sys tanf
  sys 8
  sys expf
  sys 9
  sys logf
  sys 10
  sys log2f
  sys 11
  sys log10f
  sys 12
  sys powf
  sys 13
  sys hypotf
  sys 14
  sys fmodf
  sys 15
  sys memcpy
  sys 16
  sys memcpy_p
  sys 17
  sys memset
  sys 18
  sys memmove

# ENCODING: ldi8{{.*}}encoding: [0xc0,0x50]
# ENCODING-COUNT-5: sys debug_putc{{.*}}encoding: [0xd7,0x00]
# ENCODING-COUNT-5: sys debug_break{{.*}}encoding: [0xd7,0x01]
# ENCODING-COUNT-2: sys millis{{.*}}encoding: [0xd7,0x02]
# ENCODING-COUNT-2: sys millis32{{.*}}encoding: [0xd7,0x03]
# ENCODING-COUNT-2: sys sinf{{.*}}encoding: [0xd7,0x04]
# ENCODING-COUNT-2: sys cosf{{.*}}encoding: [0xd7,0x05]
# ENCODING-COUNT-2: sys atan2f{{.*}}encoding: [0xd7,0x06]
# ENCODING-COUNT-2: sys tanf{{.*}}encoding: [0xd7,0x07]
# ENCODING-COUNT-2: sys expf{{.*}}encoding: [0xd7,0x08]
# ENCODING-COUNT-2: sys logf{{.*}}encoding: [0xd7,0x09]
# ENCODING-COUNT-2: sys log2f{{.*}}encoding: [0xd7,0x0a]
# ENCODING-COUNT-2: sys log10f{{.*}}encoding: [0xd7,0x0b]
# ENCODING-COUNT-2: sys powf{{.*}}encoding: [0xd7,0x0c]
# ENCODING-COUNT-2: sys hypotf{{.*}}encoding: [0xd7,0x0d]
# ENCODING-COUNT-2: sys fmodf{{.*}}encoding: [0xd7,0x0e]
# ENCODING-COUNT-2: sys memcpy{{.*}}encoding: [0xd7,0x0f]
# ENCODING-COUNT-2: sys memcpy_p{{.*}}encoding: [0xd7,0x10]
# ENCODING-COUNT-2: sys memset{{.*}}encoding: [0xd7,0x11]
# ENCODING-COUNT-2: sys memmove{{.*}}encoding: [0xd7,0x12]

# DISASM:      ldi8 r4, 0x50
# DISASM-NEXT: sys debug_putc
# DISASM-NEXT: sys debug_putc
# DISASM-NEXT: sys debug_putc
# DISASM-NEXT: sys debug_putc
# DISASM-NEXT: sys debug_putc
# DISASM-NEXT: sys debug_break
# DISASM-NEXT: sys debug_break
# DISASM-NEXT: sys debug_break
# DISASM-NEXT: sys debug_break
# DISASM-NEXT: sys debug_break
# DISASM-NEXT: sys millis
# DISASM-NEXT: sys millis
# DISASM-NEXT: sys millis32
# DISASM-NEXT: sys millis32
# DISASM-NEXT: sys sinf
# DISASM-NEXT: sys sinf
# DISASM-NEXT: sys cosf
# DISASM-NEXT: sys cosf
# DISASM-NEXT: sys atan2f
# DISASM-NEXT: sys atan2f
# DISASM-NEXT: sys tanf
# DISASM-NEXT: sys tanf
# DISASM-NEXT: sys expf
# DISASM-NEXT: sys expf
# DISASM-NEXT: sys logf
# DISASM-NEXT: sys logf
# DISASM-NEXT: sys log2f
# DISASM-NEXT: sys log2f
# DISASM-NEXT: sys log10f
# DISASM-NEXT: sys log10f
# DISASM-NEXT: sys powf
# DISASM-NEXT: sys powf
# DISASM-NEXT: sys hypotf
# DISASM-NEXT: sys hypotf
# DISASM-NEXT: sys fmodf
# DISASM-NEXT: sys fmodf
# DISASM-NEXT: sys memcpy
# DISASM-NEXT: sys memcpy
# DISASM-NEXT: sys memcpy_p
# DISASM-NEXT: sys memcpy_p
# DISASM-NEXT: sys memset
# DISASM-NEXT: sys memset
# DISASM-NEXT: sys memmove
# DISASM-NEXT: sys memmove
