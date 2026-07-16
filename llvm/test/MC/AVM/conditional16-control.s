# RUN: llvm-mc -triple=avm -show-encoding %s | FileCheck %s --check-prefix=ENC
# RUN: llvm-mc -triple=avm -filetype=obj %s -o %t.o
# RUN: llvm-readobj --relocations %t.o | FileCheck %s --check-prefix=RELOC
# RUN: llvm-objdump -d --triple=avm %t.o | FileCheck %s --check-prefix=DIS

breq16 -32768
brne16 -1
brult16 0
bruge16 1
brslt16 32767
brsge16 -2

breq16 external
brne16 external + 1
brult16 external - 1
bruge16 external
brslt16 external + 1
brsge16 external - 1

# ENC: breq16 -32768{{.*}}encoding: [0xda,0x00,0x80]
# ENC: brne16 -1{{.*}}encoding: [0xdb,0xff,0xff]
# ENC: brult16 0{{.*}}encoding: [0xdc,0x00,0x00]
# ENC: bruge16 1{{.*}}encoding: [0xdd,0x01,0x00]
# ENC: brslt16 32767{{.*}}encoding: [0xde,0xff,0x7f]
# ENC: brsge16 -2{{.*}}encoding: [0xdf,0xfe,0xff]

# RELOC-COUNT-6: R_AVM_PCREL16 external
# RELOC-NOT: R_AVM_RELAX

# DIS: breq16 -0x8000
# DIS: brne16 -0x1
# DIS: brult16 0x0
# DIS: bruge16 0x1
# DIS: brslt16 0x7fff
# DIS: brsge16 -0x2
