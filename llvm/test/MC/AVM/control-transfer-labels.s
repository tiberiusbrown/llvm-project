# RUN: llvm-mc -triple=avm -filetype=obj %s -o %t.o
# RUN: llvm-objdump -d --triple=avm %t.o | FileCheck %s

jmp16 forward
.byte 0
forward:
back:
.byte 0
call16 back
.zero 32760
jmp16 far_forward
.zero 32767
far_forward:
low:
.zero 32765
call16 low

# CHECK: e0 01 00{{ *}}jmp16{{ *}}1
# CHECK: e1 fc ff{{ *}}call16{{ *}}-4
# CHECK: e0 ff 7f{{ *}}jmp16{{ *}}32767
# CHECK: e1 00 80{{ *}}call16{{ *}}-32768
