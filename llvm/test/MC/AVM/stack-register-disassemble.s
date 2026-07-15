# RUN: llvm-mc -triple=avm-unknown-arduboyfx --disassemble < %S/Inputs/stack-register-b0-bf.txt | FileCheck %s

# CHECK: push16{{ *}}r0
# CHECK: push16{{ *}}r1
# CHECK: push16{{ *}}r2
# CHECK: push16{{ *}}r3
# CHECK: push16{{ *}}r4
# CHECK: push16{{ *}}r5
# CHECK: push16{{ *}}r6
# CHECK: push16{{ *}}r7
# CHECK: pop16{{ *}}r0
# CHECK: pop16{{ *}}r1
# CHECK: pop16{{ *}}r2
# CHECK: pop16{{ *}}r3
# CHECK: pop16{{ *}}r4
# CHECK: pop16{{ *}}r5
# CHECK: pop16{{ *}}r6
# CHECK: pop16{{ *}}r7
# CHECK-NOT: c[0-3]
# CHECK-NOT: q[0-3]
