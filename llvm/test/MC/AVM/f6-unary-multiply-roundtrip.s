# RUN: llvm-mc -triple=avm -filetype=obj %s -o %t.orig.o
# RUN: llvm-objdump -d --no-show-raw-insn %t.orig.o | sed -n '/^[[:space:]]*[0-9a-f][0-9a-f]*:/s/.*:[[:space:]]*//p' > %t.reassembled.s
# RUN: llvm-mc -triple=avm -filetype=obj %t.reassembled.s -o %t.reassembled.o
# RUN: llvm-objdump -s --section=.text %t.orig.o | grep '^[[:space:]]*0000' > %t.orig.txt
# RUN: llvm-objdump -s --section=.text %t.reassembled.o | grep '^[[:space:]]*0000' > %t.reassembled.txt
# RUN: diff -u %t.orig.txt %t.reassembled.txt

bswap16 r0
bswap16 r1
bswap16 r2
bswap16 r3
bswap16 r4
bswap16 r5
bswap16 r6
bswap16 r7
tst16 r0
tst16 r1
tst16 r2
tst16 r3
tst16 r4
tst16 r5
tst16 r6
tst16 r7
mul8 c0, c0
mul8 c0, c1
mul8 c0, c2
mul8 c0, c3
mul8 c1, c0
mul8 c1, c1
mul8 c1, c2
mul8 c1, c3
mul8 c2, c0
mul8 c2, c1
mul8 c2, c2
mul8 c2, c3
mul8 c3, c0
mul8 c3, c1
mul8 c3, c2
mul8 c3, c3
sext8 r0
sext8 r1
sext8 r2
sext8 r3
sext8 r4
sext8 r5
sext8 r6
sext8 r7
neg16 r0
neg16 r1
neg16 r2
neg16 r3
neg16 r4
neg16 r5
neg16 r6
neg16 r7
