# RUN: llvm-mc -triple=avm -show-encoding %s | FileCheck %s

breq8 -128
breq8 -127
breq8 -1
breq8 0
breq8 1
breq8 126
breq8 127
brne8 -128
brne8 -127
brne8 -1
brne8 0
brne8 1
brne8 126
brne8 127
brult8 -128
brult8 -127
brult8 -1
brult8 0
brult8 1
brult8 126
brult8 127
brslt8 -128
brslt8 -127
brslt8 -1
brslt8 0
brslt8 1
brslt8 126
brslt8 127
bruge8 -128
bruge8 -1
bruge8 0
bruge8 1
bruge8 127
brsge8 -128
brsge8 -1
brsge8 0
brsge8 1
brsge8 127
bruge8 0
brsge8 0
jmp8 -128
jmp8 -127
jmp8 -1
jmp8 0
jmp8 1
jmp8 126
jmp8 127
call8 -128
call8 -127
call8 -1
call8 0
call8 1
call8 126
call8 127
adjsp -128
adjsp -127
adjsp -1
adjsp 0
adjsp 1
adjsp 126
adjsp 127
sys 0
sys 1
sys 2
sys 3

# CHECK: breq8{{.*}}[0xd0,0x80]
# CHECK: breq8{{.*}}[0xd0,0x81]
# CHECK: breq8{{.*}}[0xd0,0xff]
# CHECK: breq8{{.*}}[0xd0,0x00]
# CHECK: breq8{{.*}}[0xd0,0x01]
# CHECK: breq8{{.*}}[0xd0,0x7e]
# CHECK: breq8{{.*}}[0xd0,0x7f]
# CHECK: brne8{{.*}}[0xd1,0x80]
# CHECK: brne8{{.*}}[0xd1,0x81]
# CHECK: brne8{{.*}}[0xd1,0xff]
# CHECK: brne8{{.*}}[0xd1,0x00]
# CHECK: brne8{{.*}}[0xd1,0x01]
# CHECK: brne8{{.*}}[0xd1,0x7e]
# CHECK: brne8{{.*}}[0xd1,0x7f]
# CHECK: brult8{{.*}}[0xd2,0x80]
# CHECK: brult8{{.*}}[0xd2,0x81]
# CHECK: brult8{{.*}}[0xd2,0xff]
# CHECK: brult8{{.*}}[0xd2,0x00]
# CHECK: brult8{{.*}}[0xd2,0x01]
# CHECK: brult8{{.*}}[0xd2,0x7e]
# CHECK: brult8{{.*}}[0xd2,0x7f]
# CHECK: brslt8{{.*}}[0xd3,0x80]
# CHECK: brslt8{{.*}}[0xd3,0x81]
# CHECK: brslt8{{.*}}[0xd3,0xff]
# CHECK: brslt8{{.*}}[0xd3,0x00]
# CHECK: brslt8{{.*}}[0xd3,0x01]
# CHECK: brslt8{{.*}}[0xd3,0x7e]
# CHECK: brslt8{{.*}}[0xd3,0x7f]
# CHECK: bruge8{{.*}}[0xd8,0x80]
# CHECK: bruge8{{.*}}[0xd8,0xff]
# CHECK: bruge8{{.*}}[0xd8,0x00]
# CHECK: bruge8{{.*}}[0xd8,0x01]
# CHECK: bruge8{{.*}}[0xd8,0x7f]
# CHECK: brsge8{{.*}}[0xd9,0x80]
# CHECK: brsge8{{.*}}[0xd9,0xff]
# CHECK: brsge8{{.*}}[0xd9,0x00]
# CHECK: brsge8{{.*}}[0xd9,0x01]
# CHECK: brsge8{{.*}}[0xd9,0x7f]
# CHECK: bruge8{{.*}}[0xd8,0x00]
# CHECK: brsge8{{.*}}[0xd9,0x00]
# CHECK: jmp8{{.*}}[0xd4,0x80]
# CHECK: jmp8{{.*}}[0xd4,0x81]
# CHECK: jmp8{{.*}}[0xd4,0xff]
# CHECK: jmp8{{.*}}[0xd4,0x00]
# CHECK: jmp8{{.*}}[0xd4,0x01]
# CHECK: jmp8{{.*}}[0xd4,0x7e]
# CHECK: jmp8{{.*}}[0xd4,0x7f]
# CHECK: call8{{.*}}[0xd5,0x80]
# CHECK: call8{{.*}}[0xd5,0x81]
# CHECK: call8{{.*}}[0xd5,0xff]
# CHECK: call8{{.*}}[0xd5,0x00]
# CHECK: call8{{.*}}[0xd5,0x01]
# CHECK: call8{{.*}}[0xd5,0x7e]
# CHECK: call8{{.*}}[0xd5,0x7f]
# CHECK: adjsp{{.*}}[0xd6,0x80]
# CHECK: adjsp{{.*}}[0xd6,0x81]
# CHECK: adjsp{{.*}}[0xd6,0xff]
# CHECK: adjsp{{.*}}[0xd6,0x00]
# CHECK: adjsp{{.*}}[0xd6,0x01]
# CHECK: adjsp{{.*}}[0xd6,0x7e]
# CHECK: adjsp{{.*}}[0xd6,0x7f]
# CHECK: sys{{.*}}[0xd7,0x00]
# CHECK: sys{{.*}}[0xd7,0x01]
# CHECK: sys{{.*}}[0xd7,0x02]
# CHECK: sys{{.*}}[0xd7,0x03]
