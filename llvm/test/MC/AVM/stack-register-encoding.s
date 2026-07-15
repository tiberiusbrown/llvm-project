# RUN: llvm-mc -triple=avm-unknown-arduboyfx -show-encoding %s | FileCheck %s

push16 r0
push16 r1
push16 r2
push16 r3
push16 r4
push16 r5
push16 r6
push16 r7
pop16 r0
pop16 r1
pop16 r2
pop16 r3
pop16 r4
pop16 r5
pop16 r6
pop16 r7

# CHECK: push16{{ *}}r0{{.*}}encoding: [0xb0]
# CHECK: push16{{ *}}r1{{.*}}encoding: [0xb1]
# CHECK: push16{{ *}}r2{{.*}}encoding: [0xb2]
# CHECK: push16{{ *}}r3{{.*}}encoding: [0xb3]
# CHECK: push16{{ *}}r4{{.*}}encoding: [0xb4]
# CHECK: push16{{ *}}r5{{.*}}encoding: [0xb5]
# CHECK: push16{{ *}}r6{{.*}}encoding: [0xb6]
# CHECK: push16{{ *}}r7{{.*}}encoding: [0xb7]
# CHECK: pop16{{ *}}r0{{.*}}encoding: [0xb8]
# CHECK: pop16{{ *}}r1{{.*}}encoding: [0xb9]
# CHECK: pop16{{ *}}r2{{.*}}encoding: [0xba]
# CHECK: pop16{{ *}}r3{{.*}}encoding: [0xbb]
# CHECK: pop16{{ *}}r4{{.*}}encoding: [0xbc]
# CHECK: pop16{{ *}}r5{{.*}}encoding: [0xbd]
# CHECK: pop16{{ *}}r6{{.*}}encoding: [0xbe]
# CHECK: pop16{{ *}}r7{{.*}}encoding: [0xbf]
