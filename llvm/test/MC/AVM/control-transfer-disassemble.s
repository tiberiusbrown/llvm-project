# RUN: llvm-mc -triple=avm -filetype=obj %s -o %t.o
# RUN: llvm-objdump -d --triple=avm %t.o | FileCheck %s

.byte 0xe0, 0x01, 0x00
.byte 0xe1, 0xfe, 0xff
.byte 0xe2, 0x01, 0x00, 0x00
.byte 0xe3, 0xff, 0xff, 0xff
.byte 0xe4, 0xe5, 0xe6, 0xe7, 0xe8, 0xe9, 0xea, 0xeb, 0xef
.byte 0xec, 0x00, 0xed, 0xee

# CHECK: e0 01 00{{ *}}jmp16{{ *}}1
# CHECK: e1 fe ff{{ *}}call16{{ *}}-2
# CHECK: e2 01 00 00{{ *}}jmpf{{ *}}0x1
# CHECK: e3 ff ff ff{{ *}}callf{{ *}}0xffffff
# CHECK: e4{{ *}}jmpp{{ *}}q0
# CHECK: e5{{ *}}jmpp{{ *}}q1
# CHECK: e6{{ *}}jmpp{{ *}}q2
# CHECK: e7{{ *}}jmpp{{ *}}q3
# CHECK: e8{{ *}}callp{{ *}}q0
# CHECK: e9{{ *}}callp{{ *}}q1
# CHECK: ea{{ *}}callp{{ *}}q2
# CHECK: eb{{ *}}callp{{ *}}q3
# CHECK: ef{{ *}}ret
# CHECK: ec 00{{ *}}udiv16{{ *}}r0, r0
# CHECK-COUNT-1: <unknown>
