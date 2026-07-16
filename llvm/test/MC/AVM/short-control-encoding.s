# RUN: llvm-mc -triple=avm -show-encoding %s | FileCheck %s

breq -128
breq -127
breq -1
breq 0
breq 1
breq 126
breq 127
brne -128
brne -127
brne -1
brne 0
brne 1
brne 126
brne 127
brult -128
brult -127
brult -1
brult 0
brult 1
brult 126
brult 127
brslt -128
brslt -127
brslt -1
brslt 0
brslt 1
brslt 126
brslt 127
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

# CHECK: breq{{.*}}[0xd0,0x80]
# CHECK: breq{{.*}}[0xd0,0x81]
# CHECK: breq{{.*}}[0xd0,0xff]
# CHECK: breq{{.*}}[0xd0,0x00]
# CHECK: breq{{.*}}[0xd0,0x01]
# CHECK: breq{{.*}}[0xd0,0x7e]
# CHECK: breq{{.*}}[0xd0,0x7f]
# CHECK: brne{{.*}}[0xd1,0x80]
# CHECK: brne{{.*}}[0xd1,0x81]
# CHECK: brne{{.*}}[0xd1,0xff]
# CHECK: brne{{.*}}[0xd1,0x00]
# CHECK: brne{{.*}}[0xd1,0x01]
# CHECK: brne{{.*}}[0xd1,0x7e]
# CHECK: brne{{.*}}[0xd1,0x7f]
# CHECK: brult{{.*}}[0xd2,0x80]
# CHECK: brult{{.*}}[0xd2,0x81]
# CHECK: brult{{.*}}[0xd2,0xff]
# CHECK: brult{{.*}}[0xd2,0x00]
# CHECK: brult{{.*}}[0xd2,0x01]
# CHECK: brult{{.*}}[0xd2,0x7e]
# CHECK: brult{{.*}}[0xd2,0x7f]
# CHECK: brslt{{.*}}[0xd3,0x80]
# CHECK: brslt{{.*}}[0xd3,0x81]
# CHECK: brslt{{.*}}[0xd3,0xff]
# CHECK: brslt{{.*}}[0xd3,0x00]
# CHECK: brslt{{.*}}[0xd3,0x01]
# CHECK: brslt{{.*}}[0xd3,0x7e]
# CHECK: brslt{{.*}}[0xd3,0x7f]
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
