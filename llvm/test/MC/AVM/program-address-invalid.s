# RUN: not llvm-mc -triple=avm-unknown-arduboyfx -filetype=obj %s -o %t.o 2>&1 | FileCheck %s

jmpf 0x1000000
callf 0x1000000

# CHECK-COUNT-2: error: far target is out of 24-bit range
