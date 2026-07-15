# RUN: llvm-mc -triple=avm-unknown-arduboyfx -show-encoding %s | FileCheck %s --check-prefix=ENC
# RUN: llvm-mc -triple=avm-unknown-arduboyfx -filetype=obj %s -o %t.o
# RUN: llvm-objdump -d %t.o | FileCheck %s --check-prefix=DIS

# Exact far transfers use distinct opcodes and preserve every target24 bit.
jmpf 0x123457
callf 0xabcdef

# ENC: jmpf 1193047{{.*}}encoding: [0xe2,0x57,0x34,0x12]
# ENC: callf 11259375{{.*}}encoding: [0xe3,0xef,0xcd,0xab]

# DIS: jmpf{{ *}}0x123457
# DIS: callf{{ *}}0xabcdef
