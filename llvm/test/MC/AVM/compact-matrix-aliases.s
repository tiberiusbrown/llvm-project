# RUN: llvm-mc -triple=avm-unknown-arduboyfx -show-encoding %s | FileCheck %s

nop
NOP
NoP
mov c0, c0
clr c0
clr c1
clr c2
clr c3

# CHECK-COUNT-4: nop{{.*}}encoding: [0x00]
# CHECK: xor{{.*}}r4, r4{{.*}}encoding: [0xa0]
# CHECK: xor{{.*}}r5, r5{{.*}}encoding: [0xa5]
# CHECK: xor{{.*}}r6, r6{{.*}}encoding: [0xaa]
# CHECK: xor{{.*}}r7, r7{{.*}}encoding: [0xaf]
