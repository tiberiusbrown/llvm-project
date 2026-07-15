# RUN: llvm-mc -triple=avm-unknown-arduboyfx -filetype=obj %s -o %t.orig.o
# RUN: llvm-objdump -d --no-show-raw-insn %t.orig.o | sed -n '/^[[:space:]]*[0-9a-f][0-9a-f]*:/s/.*:[[:space:]]*//p' > %t.reassembled.s
# RUN: llvm-mc -triple=avm-unknown-arduboyfx -filetype=obj %t.reassembled.s -o %t.reassembled.o
# RUN: llvm-objdump -s --section=.text %t.orig.o | grep '^[[:space:]]*0000' > %t.orig.txt
# RUN: llvm-objdump -s --section=.text %t.reassembled.o | grep '^[[:space:]]*0000' > %t.reassembled.txt
# RUN: diff -u %t.orig.txt %t.reassembled.txt

mov c0, c0
add c3, c2
sub c2, c1
cmp c3, c0
ld8u c3, [c0]
st8 [c1], c2
ld16 c2, [c3]
st16 [c0], c1
and c1, c3
or c2, c0
xor c3, c3
