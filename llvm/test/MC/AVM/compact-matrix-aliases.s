# RUN: llvm-mc -triple=avm-unknown-arduboyfx -show-encoding %s | FileCheck %s

nop
clr c0
clr c1
clr c2
clr c3

# CHECK: mov{{.*}}c0, c0{{.*}}encoding: [0x00]
# CHECK: xor{{.*}}c0, c0{{.*}}encoding: [0xa0]
# CHECK: xor{{.*}}c1, c1{{.*}}encoding: [0xa5]
# CHECK: xor{{.*}}c2, c2{{.*}}encoding: [0xaa]
# CHECK: xor{{.*}}c3, c3{{.*}}encoding: [0xaf]
