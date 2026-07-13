# RUN: not llvm-mc -triple=avm-unknown-arduboyfx -filetype=obj %s -o /dev/null 2>&1 | FileCheck %s

jmp16 0x10000
jmpf 0x1000000
callf 3

# CHECK: same-bank absolute target is out of range
# CHECK: far target is out of 24-bit range
# CHECK: far target must be even-aligned
