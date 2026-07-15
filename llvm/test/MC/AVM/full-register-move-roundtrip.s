# RUN: llvm-mc -triple=avm -filetype=obj %s -o %t.orig.o
# RUN: llvm-objdump -d --no-show-raw-insn %t.orig.o | sed -n '/^[[:space:]]*[0-9a-f][0-9a-f]*:/s/.*:[[:space:]]*//p' > %t.reassembled.s
# RUN: llvm-mc -triple=avm -filetype=obj %t.reassembled.s -o %t.reassembled.o
# RUN: llvm-objdump -s --section=.text %t.orig.o | grep '^[[:space:]]*0000' > %t.orig.txt
# RUN: llvm-objdump -s --section=.text %t.reassembled.o | grep '^[[:space:]]*0000' > %t.reassembled.txt
# RUN: diff -u %t.orig.txt %t.reassembled.txt

mov r0, r0
mov r0, r1
mov r0, r2
mov r0, r3
mov r0, r4
mov r0, r5
mov r0, r6
mov r0, r7
mov r1, r0
mov r1, r1
mov r1, r2
mov r1, r3
mov r1, r4
mov r1, r5
mov r1, r6
mov r1, r7
mov r2, r0
mov r2, r1
mov r2, r2
mov r2, r3
mov r2, r4
mov r2, r5
mov r2, r6
mov r2, r7
mov r3, r0
mov r3, r1
mov r3, r2
mov r3, r3
mov r3, r4
mov r3, r5
mov r3, r6
mov r3, r7
mov r4, r0
mov r4, r1
mov r4, r2
mov r4, r3
mov r5, r0
mov r5, r1
mov r5, r2
mov r5, r3
mov r6, r0
mov r6, r1
mov r6, r2
mov r6, r3
mov r7, r0
mov r7, r1
mov r7, r2
mov r7, r3
