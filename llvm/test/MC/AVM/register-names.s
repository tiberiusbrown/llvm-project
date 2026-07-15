# RUN: llvm-mc -triple=avm-unknown-arduboyfx -filetype=obj %s -o %t.o
# RUN: llvm-objdump -d --no-show-raw-insn %t.o | FileCheck %s

.text
push16 R0
push16 R1
push16 R2
push16 R3
push16 R4
push16 R5
push16 R6
push16 R7
ldi8 C0, 0
ldi8 C1, 0
ldi8 C2, 0
ldi8 C3, 0
jmpp Q0
jmpp Q1
jmpp Q2
jmpp Q3
ldsp8 r0, [SP+0]

# CHECK: push16 r0
# CHECK: push16 r1
# CHECK: push16 r2
# CHECK: push16 r3
# CHECK: push16 r4
# CHECK: push16 r5
# CHECK: push16 r6
# CHECK: push16 r7
# CHECK: ldi8 c0, 0x0
# CHECK: ldi8 c1, 0x0
# CHECK: ldi8 c2, 0x0
# CHECK: ldi8 c3, 0x0
# CHECK: jmpp q0
# CHECK: jmpp q1
# CHECK: jmpp q2
# CHECK: jmpp q3
# CHECK: ldsp8 r0, [sp+0x0]
