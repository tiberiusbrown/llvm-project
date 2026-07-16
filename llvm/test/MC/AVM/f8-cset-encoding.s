# RUN: llvm-mc -triple=avm -show-encoding < %s | FileCheck %s

cset.eq r0
cset.eq r1
cset.eq r2
cset.eq r3
cset.eq r4
cset.eq r5
cset.eq r6
cset.eq r7
cset.ne r0
cset.ne r1
cset.ne r2
cset.ne r3
cset.ne r4
cset.ne r5
cset.ne r6
cset.ne r7
cset.ult r0
cset.ult r1
cset.ult r2
cset.ult r3
cset.ult r4
cset.ult r5
cset.ult r6
cset.ult r7
cset.uge r0
cset.uge r1
cset.uge r2
cset.uge r3
cset.uge r4
cset.uge r5
cset.uge r6
cset.uge r7
cset.slt r0
cset.slt r1
cset.slt r2
cset.slt r3
cset.slt r4
cset.slt r5
cset.slt r6
cset.slt r7
cset.sge r0
cset.sge r1
cset.sge r2
cset.sge r3
cset.sge r4
cset.sge r5
cset.sge r6
cset.sge r7

# CHECK: cset.eq r0{{.*}}encoding: [0xf8,0x00]
# CHECK: cset.eq r1{{.*}}encoding: [0xf8,0x01]
# CHECK: cset.eq r2{{.*}}encoding: [0xf8,0x02]
# CHECK: cset.eq r3{{.*}}encoding: [0xf8,0x03]
# CHECK: cset.eq r4{{.*}}encoding: [0xf8,0x04]
# CHECK: cset.eq r5{{.*}}encoding: [0xf8,0x05]
# CHECK: cset.eq r6{{.*}}encoding: [0xf8,0x06]
# CHECK: cset.eq r7{{.*}}encoding: [0xf8,0x07]
# CHECK: cset.ne r0{{.*}}encoding: [0xf8,0x08]
# CHECK: cset.ne r1{{.*}}encoding: [0xf8,0x09]
# CHECK: cset.ne r2{{.*}}encoding: [0xf8,0x0a]
# CHECK: cset.ne r3{{.*}}encoding: [0xf8,0x0b]
# CHECK: cset.ne r4{{.*}}encoding: [0xf8,0x0c]
# CHECK: cset.ne r5{{.*}}encoding: [0xf8,0x0d]
# CHECK: cset.ne r6{{.*}}encoding: [0xf8,0x0e]
# CHECK: cset.ne r7{{.*}}encoding: [0xf8,0x0f]
# CHECK: cset.ult r0{{.*}}encoding: [0xf8,0x10]
# CHECK: cset.ult r1{{.*}}encoding: [0xf8,0x11]
# CHECK: cset.ult r2{{.*}}encoding: [0xf8,0x12]
# CHECK: cset.ult r3{{.*}}encoding: [0xf8,0x13]
# CHECK: cset.ult r4{{.*}}encoding: [0xf8,0x14]
# CHECK: cset.ult r5{{.*}}encoding: [0xf8,0x15]
# CHECK: cset.ult r6{{.*}}encoding: [0xf8,0x16]
# CHECK: cset.ult r7{{.*}}encoding: [0xf8,0x17]
# CHECK: cset.uge r0{{.*}}encoding: [0xf8,0x18]
# CHECK: cset.uge r1{{.*}}encoding: [0xf8,0x19]
# CHECK: cset.uge r2{{.*}}encoding: [0xf8,0x1a]
# CHECK: cset.uge r3{{.*}}encoding: [0xf8,0x1b]
# CHECK: cset.uge r4{{.*}}encoding: [0xf8,0x1c]
# CHECK: cset.uge r5{{.*}}encoding: [0xf8,0x1d]
# CHECK: cset.uge r6{{.*}}encoding: [0xf8,0x1e]
# CHECK: cset.uge r7{{.*}}encoding: [0xf8,0x1f]
# CHECK: cset.slt r0{{.*}}encoding: [0xf8,0x20]
# CHECK: cset.slt r1{{.*}}encoding: [0xf8,0x21]
# CHECK: cset.slt r2{{.*}}encoding: [0xf8,0x22]
# CHECK: cset.slt r3{{.*}}encoding: [0xf8,0x23]
# CHECK: cset.slt r4{{.*}}encoding: [0xf8,0x24]
# CHECK: cset.slt r5{{.*}}encoding: [0xf8,0x25]
# CHECK: cset.slt r6{{.*}}encoding: [0xf8,0x26]
# CHECK: cset.slt r7{{.*}}encoding: [0xf8,0x27]
# CHECK: cset.sge r0{{.*}}encoding: [0xf8,0x28]
# CHECK: cset.sge r1{{.*}}encoding: [0xf8,0x29]
# CHECK: cset.sge r2{{.*}}encoding: [0xf8,0x2a]
# CHECK: cset.sge r3{{.*}}encoding: [0xf8,0x2b]
# CHECK: cset.sge r4{{.*}}encoding: [0xf8,0x2c]
# CHECK: cset.sge r5{{.*}}encoding: [0xf8,0x2d]
# CHECK: cset.sge r6{{.*}}encoding: [0xf8,0x2e]
# CHECK: cset.sge r7{{.*}}encoding: [0xf8,0x2f]
