# RUN: llvm-mc -triple=avm -filetype=obj %s -o %t.orig.o
# RUN: llvm-objdump -d --no-show-raw-insn %t.orig.o | sed -n '/^[[:space:]]*[0-9a-f][0-9a-f]*:/s/.*:[[:space:]]*//p' > %t.reassembled.s
# RUN: llvm-mc -triple=avm -filetype=obj %t.reassembled.s -o %t.reassembled.o
# RUN: llvm-objdump -s --section=.text %t.orig.o | grep '^[[:space:]]*0000' > %t.orig.txt
# RUN: llvm-objdump -s --section=.text %t.reassembled.o | grep '^[[:space:]]*0000' > %t.reassembled.txt
# RUN: diff -u %t.orig.txt %t.reassembled.txt
# RUN: llvm-readobj --relocations %t.orig.o | FileCheck %s --check-prefix=NORELOC

# NORELOC: Relocations [
# NORELOC-NEXT: ]

ld8u r0, [c0]
ld8u r1, [c0]
ld8u r2, [c0]
ld8u r3, [c0]
ld8u r0, [c1]
ld8u r1, [c1]
ld8u r2, [c1]
ld8u r3, [c1]
ld8u r0, [c2]
ld8u r1, [c2]
ld8u r2, [c2]
ld8u r3, [c2]
ld8u r0, [c3]
ld8u r1, [c3]
ld8u r2, [c3]
ld8u r3, [c3]
ld16 r0, [c0]
ld16 r1, [c0]
ld16 r2, [c0]
ld16 r3, [c0]
ld16 r0, [c1]
ld16 r1, [c1]
ld16 r2, [c1]
ld16 r3, [c1]
ld16 r0, [c2]
ld16 r1, [c2]
ld16 r2, [c2]
ld16 r3, [c2]
ld16 r0, [c3]
ld16 r1, [c3]
ld16 r2, [c3]
ld16 r3, [c3]
st16 [c0], r0
st16 [c0], r1
st16 [c0], r2
st16 [c0], r3
st16 [c1], r0
st16 [c1], r1
st16 [c1], r2
st16 [c1], r3
st16 [c2], r0
st16 [c2], r1
st16 [c2], r2
st16 [c2], r3
st16 [c3], r0
st16 [c3], r1
st16 [c3], r2
st16 [c3], r3
