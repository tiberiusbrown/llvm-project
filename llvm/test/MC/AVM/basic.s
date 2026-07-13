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
  mov8z r1, b6
  mov8s r2, b5
  lsl16 r0
  lsl16 c0
  zext8 r1
  sext8 r6
  ldi8 r3, 165
  ldi8 c3, 90
  tst16 r2
  tst16 c2
  tst8 r1
  tst8 c3
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
# DIS: mov8z r1, b6
# DIS: mov8s r2, b5
# DIS: lsl16 r0
# DIS: add.nf c0, c0
# DIS: mov8z r1, b1
# DIS: mov8s r6, b6
# DIS: ldi8 r3, 0xa5
# DIS: ldi8 c3, 0x5a
# DIS: tst16 r2
# DIS: tst16 c2
# DIS: tst8 r1
# DIS: tst8 c3
# DIS: cset r3, uge
# DIS: breq
# DIS: nop
# DIS: sys 0x0
# DIS: ret

# ENC: mov16 r0, r7{{.*}}encoding: [0xe3,0x07]
# ENC: mov8z r1, b6{{.*}}encoding: [0xe3,0x4e]
# ENC: mov8s r2, b5{{.*}}encoding: [0xe3,0x95]
# ENC: lsl16 r0{{.*}}encoding: [0xe0,0x20]
# ENC: add.nf c0, c0{{.*}}encoding: [0xf4,0x10]
# ENC: mov8z r1, b1{{.*}}encoding: [0xe3,0x49]
# ENC: mov8s r6, b6{{.*}}encoding: [0xe3,0xb6]
# ENC: ldi8 r3, 165{{.*}}encoding: [0xe0,0x8b,0xa5]
# ENC: ldi8 c3, 90{{.*}}encoding: [0xf3,0x5a]
# ENC: tst16 r2{{.*}}encoding: [0xe0,0xea]
# ENC: tst16 c2{{.*}}encoding: [0xaa]
# ENC: tst8 r1{{.*}}encoding: [0xe0,0xf1]
# ENC: tst8 c3{{.*}}encoding: [0xbf]
# ENC: cset r3, uge{{.*}}encoding: [0xe3,0xdb]
