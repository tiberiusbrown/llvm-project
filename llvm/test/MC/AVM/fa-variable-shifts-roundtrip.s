# RUN: llvm-mc -triple=avm -filetype=obj %s -o %t.orig.o
# RUN: llvm-objdump -d --no-show-raw-insn %t.orig.o | sed -n '/^[[:space:]]*[0-9a-f][0-9a-f]*:/s/.*:[[:space:]]*//p' > %t.reassembled.s
# RUN: llvm-mc -triple=avm -filetype=obj %t.reassembled.s -o %t.reassembled.o
# RUN: llvm-objdump -s --section=.text %t.orig.o | grep '^[[:space:]]*0000' > %t.orig.txt
# RUN: llvm-objdump -s --section=.text %t.reassembled.o | grep '^[[:space:]]*0000' > %t.reassembled.txt
# RUN: diff -u %t.orig.txt %t.reassembled.txt
shl16v c0, c0
shl16v c0, c1
shl16v c0, c2
shl16v c0, c3
shl16v c1, c0
shl16v c1, c1
shl16v c1, c2
shl16v c1, c3
shl16v c2, c0
shl16v c2, c1
shl16v c2, c2
shl16v c2, c3
shl16v c3, c0
shl16v c3, c1
shl16v c3, c2
shl16v c3, c3
lsr16v c0, c0
lsr16v c0, c1
lsr16v c0, c2
lsr16v c0, c3
lsr16v c1, c0
lsr16v c1, c1
lsr16v c1, c2
lsr16v c1, c3
lsr16v c2, c0
lsr16v c2, c1
lsr16v c2, c2
lsr16v c2, c3
lsr16v c3, c0
lsr16v c3, c1
lsr16v c3, c2
lsr16v c3, c3
asr16v c0, c0
asr16v c0, c1
asr16v c0, c2
asr16v c0, c3
asr16v c1, c0
asr16v c1, c1
asr16v c1, c2
asr16v c1, c3
asr16v c2, c0
asr16v c2, c1
asr16v c2, c2
asr16v c2, c3
asr16v c3, c0
asr16v c3, c1
asr16v c3, c2
asr16v c3, c3
