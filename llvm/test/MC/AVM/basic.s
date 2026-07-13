# RUN: llvm-mc -triple=avm-unknown-arduboyfx -filetype=obj %s -o %t.o
# RUN: llvm-readobj --file-headers %t.o | FileCheck %s --check-prefix=ELF
# RUN: llvm-objdump -d --no-show-raw-insn %t.o | FileCheck %s --check-prefix=DIS
# RUN: llvm-mc -triple=avm-unknown-arduboyfx -show-encoding %s | FileCheck %s --check-prefix=ENC

.text
.globl _start
_start:
  ldi8 c0, 42
  add c0, c1
  ld8 c2, [c3]
  cmpi6 c0, -1
  mov16 r0, r7
  mov8z r1, r6
  mov8s r2, r5
  cset r3, uge
  breq done
  nop
done:
  sys 0
  ret

# ELF: Format: elf32-avm
# ELF: Arch: avm
# ELF: Type: Relocatable

# DIS: <_start>:
# DIS: ldi8 c0, 0x2a
# DIS: add c0, c1
# DIS: ld8 c2, [c3]
# DIS: cmpi6 c0, -0x1
# DIS: mov16 r0, r7
# DIS: mov8z r1, r6
# DIS: mov8s r2, r5
# DIS: cset r3, uge
# DIS: breq
# DIS: nop
# DIS: sys 0x0
# DIS: ret

# ENC: mov16 r0, r7{{.*}}encoding: [0xe3,0x07]
# ENC: mov8z r1, r6{{.*}}encoding: [0xe3,0x4e]
# ENC: mov8s r2, r5{{.*}}encoding: [0xe3,0x95]
# ENC: cset r3, uge{{.*}}encoding: [0xe3,0xdb]
