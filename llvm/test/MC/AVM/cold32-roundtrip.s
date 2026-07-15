# RUN: llvm-mc -triple=avm -filetype=obj %s -o %t.orig.o
# RUN: llvm-objdump -d --no-show-raw-insn %t.orig.o | sed -n '/^[[:space:]]*[0-9a-f][0-9a-f]*:/s/.*:[[:space:]]*//p' > %t.reassembled.s
# RUN: llvm-mc -triple=avm -filetype=obj %t.reassembled.s -o %t.reassembled.o
# RUN: llvm-objdump -s --section=.text %t.orig.o | grep '^[[:space:]]*0000' > %t.orig.txt
# RUN: llvm-objdump -s --section=.text %t.reassembled.o | grep '^[[:space:]]*0000' > %t.reassembled.txt
# RUN: diff -u %t.orig.txt %t.reassembled.txt
cmp32 q0,q0
cmp32 q3,q3
cmp32 q1,q2
ld32 q0,[r0]
ld32 q0,[r1]
ld32 q2,[r4]
ld32 q2,[r5]
st32 [r0],q0
st32 [r1],q0
st32 [r4],q2
st32 [r5],q2
