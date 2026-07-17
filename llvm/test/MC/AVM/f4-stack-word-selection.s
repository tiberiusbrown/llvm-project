# RUN: llvm-mc -triple=avm-unknown-arduboyfx -show-encoding %s | FileCheck %s

ldsp16 c0, [sp+(1+2)]
ldsp16 c1, [sp+8-1]
ldsp16 c2, [sp+(4*3)]
ldsp16 c3, [sp+15]
stsp16 [sp+(1+2)], c0
stsp16 [sp+8-1], c1
stsp16 [sp+(4*3)], c2
stsp16 [sp+15], c3
ldsp16 r0, [sp+0]
ldsp16 r4, [sp+0]
stsp16 [sp+0], r0
stsp16 [sp+0], r4
ldsp16 C3, [SP+15]
stsp16 [SP+15], C3

# CHECK: ldsp16 r4, [sp+3]{{.*}}encoding: [0xf4,0x0c]
# CHECK: ldsp16 r5, [sp+7]{{.*}}encoding: [0xf4,0x1d]
# CHECK: ldsp16 r6, [sp+12]{{.*}}encoding: [0xf4,0x32]
# CHECK: ldsp16 r7, [sp+15]{{.*}}encoding: [0xf4,0x3f]
# CHECK: stsp16 [sp+3], r4{{.*}}encoding: [0xf4,0x4c]
# CHECK: stsp16 [sp+7], r5{{.*}}encoding: [0xf4,0x5d]
# CHECK: stsp16 [sp+12], r6{{.*}}encoding: [0xf4,0x72]
# CHECK: stsp16 [sp+15], r7{{.*}}encoding: [0xf4,0x7f]
# CHECK: ldsp16 r0, [sp+0]{{.*}}encoding: [0xf0,0x30,0x00]
# CHECK: ldsp16 r4, [sp+0]{{.*}}encoding: [0xf4,0x00]
# CHECK: stsp16 [sp+0], r0{{.*}}encoding: [0xf0,0x38,0x00]
# CHECK: stsp16 [sp+0], r4{{.*}}encoding: [0xf4,0x40]
# CHECK: ldsp16 r7, [sp+15]{{.*}}encoding: [0xf4,0x3f]
# CHECK: stsp16 [sp+15], r7{{.*}}encoding: [0xf4,0x7f]
