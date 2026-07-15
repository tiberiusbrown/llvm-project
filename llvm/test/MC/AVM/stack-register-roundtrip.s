# RUN: llvm-mc -triple=avm-unknown-arduboyfx -filetype=obj %s -o %t.orig.o
# RUN: llvm-objdump -d --no-show-raw-insn %t.orig.o | sed -n '/^[[:space:]]*[0-9a-f][0-9a-f]*:/s/.*:[[:space:]]*//p' > %t.reassembled.s
# RUN: llvm-mc -triple=avm-unknown-arduboyfx -filetype=obj %t.reassembled.s -o %t.reassembled.o
# RUN: llvm-objdump -s --section=.text %t.orig.o | grep '^[[:space:]]*0000' > %t.orig.txt
# RUN: llvm-objdump -s --section=.text %t.reassembled.o | grep '^[[:space:]]*0000' > %t.reassembled.txt
# RUN: diff -u %t.orig.txt %t.reassembled.txt

push16 r0
push16 r1
push16 r2
push16 r3
push16 r4
push16 r5
push16 r6
push16 r7
pop16 r0
pop16 r1
pop16 r2
pop16 r3
pop16 r4
pop16 r5
pop16 r6
pop16 r7
