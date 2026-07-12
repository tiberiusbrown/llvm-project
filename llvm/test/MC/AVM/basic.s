# RUN: llvm-mc -triple=avm-unknown-arduboyfx -filetype=obj %s -o %t.o
# RUN: llvm-readobj --file-headers %t.o | FileCheck %s --check-prefix=ELF
# RUN: llvm-objdump -d --no-show-raw-insn %t.o | FileCheck %s --check-prefix=DIS

.text
.globl _start
_start:
  ldi8 c0, 42
  add c0, c1
  ld8 c2, [c3]
  cmpi6 c0, -1
  breq done
  nop
done:
  sys 0
  ret

# ELF: Format: elf32-avm
# ELF: Arch: avm
# ELF: Type: Relocatable

# DIS: <_start>:
# DIS: ldi8 c0, 42
# DIS: add c0, c1
# DIS: ld8 c2, [c3]
# DIS: cmpi6 c0, -1
# DIS: breq
# DIS: nop
# DIS: sys 0
# DIS: ret
