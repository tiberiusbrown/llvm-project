# RUN: not llvm-mc -triple=avm-unknown-arduboyfx -filetype=obj %s -o /dev/null 2>&1 | FileCheck %s

jmp16 0x10000
jmpf 0x1000000
callf 3
jmpp r0
callp r7
jmpp c0
callp c3
jmpp q4
callp r0:r1

# CHECK: same-bank absolute target is out of range
# CHECK: far target is out of 24-bit range
# CHECK: far target must be even-aligned
# CHECK-COUNT-6: error: expected AVM register pair q0-q3
