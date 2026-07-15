# RUN: llvm-mc -triple=avm -filetype=obj %s -o %t.orig.o
# RUN: llvm-objdump -d --no-show-raw-insn %t.orig.o | sed -n '/^[[:space:]]*[0-9a-f][0-9a-f]*:/s/.*:[[:space:]]*//p' > %t.reassembled.s
# RUN: llvm-mc -triple=avm -filetype=obj %t.reassembled.s -o %t.reassembled.o
# RUN: llvm-objdump -s --section=.text %t.orig.o | grep '^[[:space:]]*0000' > %t.orig.txt
# RUN: llvm-objdump -s --section=.text %t.reassembled.o | grep '^[[:space:]]*0000' > %t.reassembled.txt
# RUN: diff -u %t.orig.txt %t.reassembled.txt

jmp16 -32768
jmp16 -1
jmp16 0
jmp16 32767
call16 -32768
call16 -1
call16 0
call16 32767
jmpf 0
jmpf 1
jmpf 0xffffff
callf 0
callf 1
callf 0xffffff
jmpp q0
jmpp q1
jmpp q2
jmpp q3
callp q0
callp q1
callp q2
callp q3
ret
