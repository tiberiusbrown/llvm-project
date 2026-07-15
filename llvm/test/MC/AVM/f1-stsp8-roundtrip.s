# RUN: llvm-mc -triple=avm -filetype=obj %s -o %t.orig.o
# RUN: llvm-objdump -d --no-show-raw-insn %t.orig.o | sed -n '/^[[:space:]]*[0-9a-f][0-9a-f]*:/s/.*:[[:space:]]*//p' > %t.reassembled.s
# RUN: llvm-mc -triple=avm -filetype=obj %t.reassembled.s -o %t.reassembled.o
# RUN: llvm-objdump -s --section=.text %t.orig.o | grep '^[[:space:]]*0000' > %t.orig.txt
# RUN: llvm-objdump -s --section=.text %t.reassembled.o | grep '^[[:space:]]*0000' > %t.reassembled.txt
# RUN: diff -u %t.orig.txt %t.reassembled.txt

stsp8 [sp+0], c0
stsp8 [sp+0], c1
stsp8 [sp+0], c2
stsp8 [sp+0], c3
stsp8 [sp+1], c0
stsp8 [sp+1], c1
stsp8 [sp+1], c2
stsp8 [sp+1], c3
stsp8 [sp+2], c0
stsp8 [sp+2], c1
stsp8 [sp+2], c2
stsp8 [sp+2], c3
stsp8 [sp+3], c0
stsp8 [sp+3], c1
stsp8 [sp+3], c2
stsp8 [sp+3], c3
stsp8 [sp+4], c0
stsp8 [sp+4], c1
stsp8 [sp+4], c2
stsp8 [sp+4], c3
stsp8 [sp+5], c0
stsp8 [sp+5], c1
stsp8 [sp+5], c2
stsp8 [sp+5], c3
stsp8 [sp+6], c0
stsp8 [sp+6], c1
stsp8 [sp+6], c2
stsp8 [sp+6], c3
stsp8 [sp+7], c0
stsp8 [sp+7], c1
stsp8 [sp+7], c2
stsp8 [sp+7], c3
stsp8 [sp+8], c0
stsp8 [sp+8], c1
stsp8 [sp+8], c2
stsp8 [sp+8], c3
stsp8 [sp+9], c0
stsp8 [sp+9], c1
stsp8 [sp+9], c2
stsp8 [sp+9], c3
stsp8 [sp+10], c0
stsp8 [sp+10], c1
stsp8 [sp+10], c2
stsp8 [sp+10], c3
stsp8 [sp+11], c0
stsp8 [sp+11], c1
stsp8 [sp+11], c2
stsp8 [sp+11], c3
stsp8 [sp+12], c0
stsp8 [sp+12], c1
stsp8 [sp+12], c2
stsp8 [sp+12], c3
stsp8 [sp+13], c0
stsp8 [sp+13], c1
stsp8 [sp+13], c2
stsp8 [sp+13], c3
stsp8 [sp+14], c0
stsp8 [sp+14], c1
stsp8 [sp+14], c2
stsp8 [sp+14], c3
stsp8 [sp+15], c0
stsp8 [sp+15], c1
stsp8 [sp+15], c2
stsp8 [sp+15], c3
