# RUN: llvm-mc -triple=avm -filetype=obj %s -o %t.orig.o
# RUN: llvm-objdump -d --no-show-raw-insn %t.orig.o | sed -n '/^[[:space:]]*[0-9a-f][0-9a-f]*:/s/.*:[[:space:]]*//p' > %t.reassembled.s
# RUN: llvm-mc -triple=avm -filetype=obj %t.reassembled.s -o %t.reassembled.o
# RUN: llvm-objdump -s --section=.text %t.orig.o | grep '^[[:space:]]*0000' > %t.orig.txt
# RUN: llvm-objdump -s --section=.text %t.reassembled.o | grep '^[[:space:]]*0000' > %t.reassembled.txt
# RUN: diff -u %t.orig.txt %t.reassembled.txt

ldi8 r0, 0
ldi8 r3, 255
ldi16 r0, 1
ldi16 r3, 0x1234
addi.s8 r0, -128
addi.s8 r3, 127
cmpi.s8 r0, -1
cmpi.s8 r3, 0
leasp r0, 0
leasp r7, 255
ldsp8u r0, [sp+0]
ldsp8u r7, [sp+255]
ldsp8s r0, [sp+0]
ldsp8s r7, [sp+255]
stsp8 [sp+0], r0
stsp8 [sp+255], r7
ldsp16 r0, [sp+0]
ldsp16 r7, [sp+255]
stsp16 [sp+0], r0
stsp16 [sp+255], r7
