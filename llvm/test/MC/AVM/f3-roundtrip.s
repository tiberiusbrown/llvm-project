# RUN: llvm-mc -triple=avm-unknown-arduboyfx -filetype=obj %s -o %t.orig.o
# RUN: llvm-objdump -d --no-show-raw-insn %t.orig.o | sed -n '/^[[:space:]]*[0-9a-f][0-9a-f]*:/s/.*:[[:space:]]*//p' > %t.reassembled.s
# RUN: llvm-mc -triple=avm-unknown-arduboyfx -filetype=obj %t.reassembled.s -o %t.reassembled.o
# RUN: llvm-objdump -s --section=.text %t.orig.o | grep '^[[:space:]]*0000' > %t.orig.txt
# RUN: llvm-objdump -s --section=.text %t.reassembled.o | grep '^[[:space:]]*0000' > %t.reassembled.txt
# RUN: diff -u %t.orig.txt %t.reassembled.txt

st8 [c0], r0
st8 [c0], r1
st8 [c0], r2
st8 [c0], r3
st8 [c1], r0
st8 [c1], r1
st8 [c1], r2
st8 [c1], r3
st8 [c2], r0
st8 [c2], r1
st8 [c2], r2
st8 [c2], r3
st8 [c3], r0
st8 [c3], r1
st8 [c3], r2
st8 [c3], r3
mulu8.w c0, c0
mulu8.w c0, c1
mulu8.w c0, c2
mulu8.w c0, c3
mulu8.w c1, c0
mulu8.w c1, c1
mulu8.w c1, c2
mulu8.w c1, c3
mulu8.w c2, c0
mulu8.w c2, c1
mulu8.w c2, c2
mulu8.w c2, c3
mulu8.w c3, c0
mulu8.w c3, c1
mulu8.w c3, c2
mulu8.w c3, c3
muls8.w c0, c0
muls8.w c0, c1
muls8.w c0, c2
muls8.w c0, c3
muls8.w c1, c0
muls8.w c1, c1
muls8.w c1, c2
muls8.w c1, c3
muls8.w c2, c0
muls8.w c2, c1
muls8.w c2, c2
muls8.w c2, c3
muls8.w c3, c0
muls8.w c3, c1
muls8.w c3, c2
muls8.w c3, c3
mulsu8.w c0, c0
mulsu8.w c0, c1
mulsu8.w c0, c2
mulsu8.w c0, c3
mulsu8.w c1, c0
mulsu8.w c1, c1
mulsu8.w c1, c2
mulsu8.w c1, c3
mulsu8.w c2, c0
mulsu8.w c2, c1
mulsu8.w c2, c2
mulsu8.w c2, c3
mulsu8.w c3, c0
mulsu8.w c3, c1
mulsu8.w c3, c2
mulsu8.w c3, c3

