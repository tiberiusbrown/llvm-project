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
  mov a, r0
  mov c1, c1
  mov16 c0, c1
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
  cmp16 c0, c1
  cmp8 c2, c3
  cmp16 a, r0
  cmp8 a, r3
  sub.nf c2, c2
  add.nf c3, c3
  sub.nf c0, c1
  mulu8 c0, c0
  muls8 c1, c2
  mulsu8 c2, c3
  shl16v c3, c0
  lsr16v c0, c3
  asr16v c3, c3
  ld8_post c0, [c1]+
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
# DIS: mov16 r4, r0
# DIS: nop
# DIS: mov16 r4, r5
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
# DIS: cmp16 c0, c1
# DIS: cmp8 c2, c3
# DIS: cmp16 r4, r0
# DIS: cmp8 r4, r3
# DIS: clr c2
# DIS: add.nf c3, c3
# DIS: sub.nf c0, c1
# DIS: mulu8 r4, r4
# DIS: muls8 r5, r6
# DIS: mulsu8 r6, r7
# DIS: shl16v r7, r4
# DIS: lsr16v r4, r7
# DIS: asr16v r7, r7
# DIS: ld8_post r4, [r5]+
# DIS: cset r3, uge
# DIS: breq
# DIS: nop
# DIS: sys 0x0
# DIS: ret

# ENC: mov16 r0, r7{{.*}}encoding: [0xe3,0x07]
# ENC: mov16 r4, r0{{.*}}encoding: [0xe3,0x20]
# ENC: nop{{.*}}encoding: [0xec]
# ENC: mov16 r4, r5{{.*}}encoding: [0xe3,0x25]
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
# ENC: cmp16 c0, c1{{.*}}encoding: [0xa1]
# ENC: cmp8 c2, c3{{.*}}encoding: [0xbb]
# ENC: cmp16 r4, r0{{.*}}encoding: [0xe2,0x0c]
# ENC: cmp8 r4, r3{{.*}}encoding: [0xe2,0x13]
# ENC: clr c2{{.*}}encoding: [0x0a]
# ENC: add.nf c3, c3{{.*}}encoding: [0xf4,0x1f]
# ENC: sub.nf c0, c1{{.*}}encoding: [0xf4,0x21]
# ENC: mulu8 r4, r4{{.*}}encoding: [0xf4,0x90]
# ENC: muls8 r5, r6{{.*}}encoding: [0xf4,0xa6]
# ENC: mulsu8 r6, r7{{.*}}encoding: [0xf4,0xbb]
# ENC: shl16v r7, r4{{.*}}encoding: [0xf4,0xcc]
# ENC: lsr16v r4, r7{{.*}}encoding: [0xf4,0xd3]
# ENC: asr16v r7, r7{{.*}}encoding: [0xf4,0xef]
# ENC: ld8_post r4, [r5]+{{.*}}encoding: [0xfd,0x04,0x8a]
# ENC: cset r3, uge{{.*}}encoding: [0xe3,0xdb]
