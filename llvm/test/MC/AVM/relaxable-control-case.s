# RUN: llvm-mc -triple=avm -show-encoding %s | FileCheck %s

JMP target
CALL target
breq target
brne target
brult target
bruge target
brslt target
brsge target

# CHECK: jmp target{{.*}}[0xe2,B,B,B]
# CHECK: call target{{.*}}[0xe3,B,B,B]
# CHECK: breq target{{.*}}[0xd1,0x04,0xe2,B,B,B]
# CHECK: brne target{{.*}}[0xd0,0x04,0xe2,B,B,B]
# CHECK: brult target{{.*}}[0xd8,0x04,0xe2,B,B,B]
# CHECK: bruge target{{.*}}[0xd2,0x04,0xe2,B,B,B]
# CHECK: brslt target{{.*}}[0xd9,0x04,0xe2,B,B,B]
# CHECK: brsge target{{.*}}[0xd3,0x04,0xe2,B,B,B]
