# RUN: not llvm-mc -triple=avm -show-encoding %s 2>&1 | FileCheck %s

jmp16 -32769
jmp16 32768
jmp16 65535
jmp16 0xffff
call16 -32769
call16 32768
call16 65535
call16 0xffff
jmpf -1
callf 0x1000000
jmpp r0
jmpp c0
jmpp sp
jmpp 0
jmpp [q0]
jmpp symbol
callp r0:r1
callp 1
ret q0
ret 0
ret symbol
ret,

# CHECK-COUNT-8: error: relative displacement is out of signed 16-bit range
# CHECK-COUNT-2: error: far target is out of 24-bit range
# CHECK-COUNT-7: error: expected program pair q0-q3
# CHECK-COUNT-3: error: unexpected token after AVM instruction
