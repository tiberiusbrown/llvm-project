# RUN: llvm-mc -triple=avm -filetype=obj %s -o %t.o
# RUN: llvm-readobj --symbols %t.o | FileCheck %s

jmp after_jmp
after_jmp:
call after_call
after_call:
breq after_eq
after_eq:
brne after_ne
after_ne:
brult after_ult
after_ult:
bruge after_uge
after_uge:
brslt after_slt
after_slt:
brsge after_sge
after_sge:

# CHECK: Name: after_jmp
# CHECK: Value: 0x4
# CHECK: Name: after_call
# CHECK: Value: 0x8
# CHECK: Name: after_eq
# CHECK: Value: 0xE
# CHECK: Name: after_ne
# CHECK: Value: 0x14
# CHECK: Name: after_ult
# CHECK: Value: 0x1A
# CHECK: Name: after_uge
# CHECK: Value: 0x20
# CHECK: Name: after_slt
# CHECK: Value: 0x26
# CHECK: Name: after_sge
# CHECK: Value: 0x2C
