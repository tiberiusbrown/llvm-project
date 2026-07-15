# RUN: llvm-mc -triple=avm --disassemble < %S/Inputs/program-load-all-pspec.txt | FileCheck %s
# RUN: llvm-mc -triple=avm --disassemble < %S/Inputs/program-load-truncated-f0.txt 2>&1 | FileCheck --check-prefix=TRUNC %s
# RUN: llvm-mc -triple=avm --disassemble < %S/Inputs/program-load-truncated-60.txt 2>&1 | FileCheck --check-prefix=TRUNC %s
# RUN: llvm-mc -triple=avm --disassemble < %S/Inputs/program-load-truncated-61.txt 2>&1 | FileCheck --check-prefix=TRUNC %s
# RUN: llvm-mc -triple=avm --disassemble < %S/Inputs/program-load-truncated-62.txt 2>&1 | FileCheck --check-prefix=TRUNC %s
# RUN: llvm-mc -triple=avm --disassemble < %S/Inputs/program-load-truncated-63.txt 2>&1 | FileCheck --check-prefix=TRUNC %s
# RUN: llvm-mc -triple=avm --disassemble < %S/Inputs/program-load-truncated-64.txt 2>&1 | FileCheck --check-prefix=TRUNC %s
# RUN: llvm-mc -triple=avm --disassemble < %S/Inputs/program-load-truncated-65.txt 2>&1 | FileCheck --check-prefix=TRUNC %s
# RUN: llvm-mc -triple=avm --disassemble < %S/Inputs/program-load-truncated-66.txt 2>&1 | FileCheck --check-prefix=TRUNC %s
# RUN: llvm-mc -triple=avm --disassemble < %S/Inputs/program-load-truncated-67.txt 2>&1 | FileCheck --check-prefix=TRUNC %s
# RUN: llvm-mc -triple=avm --disassemble < %S/Inputs/program-load-truncated-68.txt 2>&1 | FileCheck --check-prefix=TRUNC %s

# Each F0 60-68 PSPEC is present exactly once in the input.  Only the 200
# structurally valid, architecturally permitted combinations may decode.
# CHECK-NOT: ldp
# CHECK: ldp8u{{[ \t]+}}r0, [q0]
# CHECK-NOT: ldp
# CHECK: ldp8u{{[ \t]+}}r0, [q1]
# CHECK-NOT: ldp
# CHECK: ldp8u{{[ \t]+}}r0, [q2]
# CHECK-NOT: ldp
# CHECK: ldp8u{{[ \t]+}}r0, [q3]
# CHECK-NOT: ldp
# CHECK: ldp8u{{[ \t]+}}r1, [q0]
# CHECK-NOT: ldp
# CHECK: ldp8u{{[ \t]+}}r1, [q1]
# CHECK-NOT: ldp
# CHECK: ldp8u{{[ \t]+}}r1, [q2]
# CHECK-NOT: ldp
# CHECK: ldp8u{{[ \t]+}}r1, [q3]
# CHECK-NOT: ldp
# CHECK: ldp8u{{[ \t]+}}r2, [q0]
# CHECK-NOT: ldp
# CHECK: ldp8u{{[ \t]+}}r2, [q1]
# CHECK-NOT: ldp
# CHECK: ldp8u{{[ \t]+}}r2, [q2]
# CHECK-NOT: ldp
# CHECK: ldp8u{{[ \t]+}}r2, [q3]
# CHECK-NOT: ldp
# CHECK: ldp8u{{[ \t]+}}r3, [q0]
# CHECK-NOT: ldp
# CHECK: ldp8u{{[ \t]+}}r3, [q1]
# CHECK-NOT: ldp
# CHECK: ldp8u{{[ \t]+}}r3, [q2]
# CHECK-NOT: ldp
# CHECK: ldp8u{{[ \t]+}}r3, [q3]
# CHECK-NOT: ldp
# CHECK: ldp8u{{[ \t]+}}r4, [q0]
# CHECK-NOT: ldp
# CHECK: ldp8u{{[ \t]+}}r4, [q1]
# CHECK-NOT: ldp
# CHECK: ldp8u{{[ \t]+}}r4, [q2]
# CHECK-NOT: ldp
# CHECK: ldp8u{{[ \t]+}}r4, [q3]
# CHECK-NOT: ldp
# CHECK: ldp8u{{[ \t]+}}r5, [q0]
# CHECK-NOT: ldp
# CHECK: ldp8u{{[ \t]+}}r5, [q1]
# CHECK-NOT: ldp
# CHECK: ldp8u{{[ \t]+}}r5, [q2]
# CHECK-NOT: ldp
# CHECK: ldp8u{{[ \t]+}}r5, [q3]
# CHECK-NOT: ldp
# CHECK: ldp8u{{[ \t]+}}r6, [q0]
# CHECK-NOT: ldp
# CHECK: ldp8u{{[ \t]+}}r6, [q1]
# CHECK-NOT: ldp
# CHECK: ldp8u{{[ \t]+}}r6, [q2]
# CHECK-NOT: ldp
# CHECK: ldp8u{{[ \t]+}}r6, [q3]
# CHECK-NOT: ldp
# CHECK: ldp8u{{[ \t]+}}r7, [q0]
# CHECK-NOT: ldp
# CHECK: ldp8u{{[ \t]+}}r7, [q1]
# CHECK-NOT: ldp
# CHECK: ldp8u{{[ \t]+}}r7, [q2]
# CHECK-NOT: ldp
# CHECK: ldp8u{{[ \t]+}}r7, [q3]
# CHECK-NOT: ldp
# CHECK: ldp8s{{[ \t]+}}r0, [q0]
# CHECK-NOT: ldp
# CHECK: ldp8s{{[ \t]+}}r0, [q1]
# CHECK-NOT: ldp
# CHECK: ldp8s{{[ \t]+}}r0, [q2]
# CHECK-NOT: ldp
# CHECK: ldp8s{{[ \t]+}}r0, [q3]
# CHECK-NOT: ldp
# CHECK: ldp8s{{[ \t]+}}r1, [q0]
# CHECK-NOT: ldp
# CHECK: ldp8s{{[ \t]+}}r1, [q1]
# CHECK-NOT: ldp
# CHECK: ldp8s{{[ \t]+}}r1, [q2]
# CHECK-NOT: ldp
# CHECK: ldp8s{{[ \t]+}}r1, [q3]
# CHECK-NOT: ldp
# CHECK: ldp8s{{[ \t]+}}r2, [q0]
# CHECK-NOT: ldp
# CHECK: ldp8s{{[ \t]+}}r2, [q1]
# CHECK-NOT: ldp
# CHECK: ldp8s{{[ \t]+}}r2, [q2]
# CHECK-NOT: ldp
# CHECK: ldp8s{{[ \t]+}}r2, [q3]
# CHECK-NOT: ldp
# CHECK: ldp8s{{[ \t]+}}r3, [q0]
# CHECK-NOT: ldp
# CHECK: ldp8s{{[ \t]+}}r3, [q1]
# CHECK-NOT: ldp
# CHECK: ldp8s{{[ \t]+}}r3, [q2]
# CHECK-NOT: ldp
# CHECK: ldp8s{{[ \t]+}}r3, [q3]
# CHECK-NOT: ldp
# CHECK: ldp8s{{[ \t]+}}r4, [q0]
# CHECK-NOT: ldp
# CHECK: ldp8s{{[ \t]+}}r4, [q1]
# CHECK-NOT: ldp
# CHECK: ldp8s{{[ \t]+}}r4, [q2]
# CHECK-NOT: ldp
# CHECK: ldp8s{{[ \t]+}}r4, [q3]
# CHECK-NOT: ldp
# CHECK: ldp8s{{[ \t]+}}r5, [q0]
# CHECK-NOT: ldp
# CHECK: ldp8s{{[ \t]+}}r5, [q1]
# CHECK-NOT: ldp
# CHECK: ldp8s{{[ \t]+}}r5, [q2]
# CHECK-NOT: ldp
# CHECK: ldp8s{{[ \t]+}}r5, [q3]
# CHECK-NOT: ldp
# CHECK: ldp8s{{[ \t]+}}r6, [q0]
# CHECK-NOT: ldp
# CHECK: ldp8s{{[ \t]+}}r6, [q1]
# CHECK-NOT: ldp
# CHECK: ldp8s{{[ \t]+}}r6, [q2]
# CHECK-NOT: ldp
# CHECK: ldp8s{{[ \t]+}}r6, [q3]
# CHECK-NOT: ldp
# CHECK: ldp8s{{[ \t]+}}r7, [q0]
# CHECK-NOT: ldp
# CHECK: ldp8s{{[ \t]+}}r7, [q1]
# CHECK-NOT: ldp
# CHECK: ldp8s{{[ \t]+}}r7, [q2]
# CHECK-NOT: ldp
# CHECK: ldp8s{{[ \t]+}}r7, [q3]
# CHECK-NOT: ldp
# CHECK: ldp16{{[ \t]+}}r0, [q0]
# CHECK-NOT: ldp
# CHECK: ldp16{{[ \t]+}}r0, [q1]
# CHECK-NOT: ldp
# CHECK: ldp16{{[ \t]+}}r0, [q2]
# CHECK-NOT: ldp
# CHECK: ldp16{{[ \t]+}}r0, [q3]
# CHECK-NOT: ldp
# CHECK: ldp16{{[ \t]+}}r1, [q0]
# CHECK-NOT: ldp
# CHECK: ldp16{{[ \t]+}}r1, [q1]
# CHECK-NOT: ldp
# CHECK: ldp16{{[ \t]+}}r1, [q2]
# CHECK-NOT: ldp
# CHECK: ldp16{{[ \t]+}}r1, [q3]
# CHECK-NOT: ldp
# CHECK: ldp16{{[ \t]+}}r2, [q0]
# CHECK-NOT: ldp
# CHECK: ldp16{{[ \t]+}}r2, [q1]
# CHECK-NOT: ldp
# CHECK: ldp16{{[ \t]+}}r2, [q2]
# CHECK-NOT: ldp
# CHECK: ldp16{{[ \t]+}}r2, [q3]
# CHECK-NOT: ldp
# CHECK: ldp16{{[ \t]+}}r3, [q0]
# CHECK-NOT: ldp
# CHECK: ldp16{{[ \t]+}}r3, [q1]
# CHECK-NOT: ldp
# CHECK: ldp16{{[ \t]+}}r3, [q2]
# CHECK-NOT: ldp
# CHECK: ldp16{{[ \t]+}}r3, [q3]
# CHECK-NOT: ldp
# CHECK: ldp16{{[ \t]+}}r4, [q0]
# CHECK-NOT: ldp
# CHECK: ldp16{{[ \t]+}}r4, [q1]
# CHECK-NOT: ldp
# CHECK: ldp16{{[ \t]+}}r4, [q2]
# CHECK-NOT: ldp
# CHECK: ldp16{{[ \t]+}}r4, [q3]
# CHECK-NOT: ldp
# CHECK: ldp16{{[ \t]+}}r5, [q0]
# CHECK-NOT: ldp
# CHECK: ldp16{{[ \t]+}}r5, [q1]
# CHECK-NOT: ldp
# CHECK: ldp16{{[ \t]+}}r5, [q2]
# CHECK-NOT: ldp
# CHECK: ldp16{{[ \t]+}}r5, [q3]
# CHECK-NOT: ldp
# CHECK: ldp16{{[ \t]+}}r6, [q0]
# CHECK-NOT: ldp
# CHECK: ldp16{{[ \t]+}}r6, [q1]
# CHECK-NOT: ldp
# CHECK: ldp16{{[ \t]+}}r6, [q2]
# CHECK-NOT: ldp
# CHECK: ldp16{{[ \t]+}}r6, [q3]
# CHECK-NOT: ldp
# CHECK: ldp16{{[ \t]+}}r7, [q0]
# CHECK-NOT: ldp
# CHECK: ldp16{{[ \t]+}}r7, [q1]
# CHECK-NOT: ldp
# CHECK: ldp16{{[ \t]+}}r7, [q2]
# CHECK-NOT: ldp
# CHECK: ldp16{{[ \t]+}}r7, [q3]
# CHECK-NOT: ldp
# CHECK: ldp24{{[ \t]+}}q0, [q0]
# CHECK-NOT: ldp
# CHECK: ldp24{{[ \t]+}}q0, [q1]
# CHECK-NOT: ldp
# CHECK: ldp24{{[ \t]+}}q0, [q2]
# CHECK-NOT: ldp
# CHECK: ldp24{{[ \t]+}}q0, [q3]
# CHECK-NOT: ldp
# CHECK: ldp24{{[ \t]+}}q1, [q0]
# CHECK-NOT: ldp
# CHECK: ldp24{{[ \t]+}}q1, [q1]
# CHECK-NOT: ldp
# CHECK: ldp24{{[ \t]+}}q1, [q2]
# CHECK-NOT: ldp
# CHECK: ldp24{{[ \t]+}}q1, [q3]
# CHECK-NOT: ldp
# CHECK: ldp24{{[ \t]+}}q2, [q0]
# CHECK-NOT: ldp
# CHECK: ldp24{{[ \t]+}}q2, [q1]
# CHECK-NOT: ldp
# CHECK: ldp24{{[ \t]+}}q2, [q2]
# CHECK-NOT: ldp
# CHECK: ldp24{{[ \t]+}}q2, [q3]
# CHECK-NOT: ldp
# CHECK: ldp24{{[ \t]+}}q3, [q0]
# CHECK-NOT: ldp
# CHECK: ldp24{{[ \t]+}}q3, [q1]
# CHECK-NOT: ldp
# CHECK: ldp24{{[ \t]+}}q3, [q2]
# CHECK-NOT: ldp
# CHECK: ldp24{{[ \t]+}}q3, [q3]
# CHECK-NOT: ldp
# CHECK: ldp32{{[ \t]+}}q0, [q0]
# CHECK-NOT: ldp
# CHECK: ldp32{{[ \t]+}}q0, [q1]
# CHECK-NOT: ldp
# CHECK: ldp32{{[ \t]+}}q0, [q2]
# CHECK-NOT: ldp
# CHECK: ldp32{{[ \t]+}}q0, [q3]
# CHECK-NOT: ldp
# CHECK: ldp32{{[ \t]+}}q1, [q0]
# CHECK-NOT: ldp
# CHECK: ldp32{{[ \t]+}}q1, [q1]
# CHECK-NOT: ldp
# CHECK: ldp32{{[ \t]+}}q1, [q2]
# CHECK-NOT: ldp
# CHECK: ldp32{{[ \t]+}}q1, [q3]
# CHECK-NOT: ldp
# CHECK: ldp32{{[ \t]+}}q2, [q0]
# CHECK-NOT: ldp
# CHECK: ldp32{{[ \t]+}}q2, [q1]
# CHECK-NOT: ldp
# CHECK: ldp32{{[ \t]+}}q2, [q2]
# CHECK-NOT: ldp
# CHECK: ldp32{{[ \t]+}}q2, [q3]
# CHECK-NOT: ldp
# CHECK: ldp32{{[ \t]+}}q3, [q0]
# CHECK-NOT: ldp
# CHECK: ldp32{{[ \t]+}}q3, [q1]
# CHECK-NOT: ldp
# CHECK: ldp32{{[ \t]+}}q3, [q2]
# CHECK-NOT: ldp
# CHECK: ldp32{{[ \t]+}}q3, [q3]
# CHECK-NOT: ldp
# CHECK: ldp8u{{[ \t]+}}r0, [q1+]
# CHECK-NOT: ldp
# CHECK: ldp8u{{[ \t]+}}r0, [q2+]
# CHECK-NOT: ldp
# CHECK: ldp8u{{[ \t]+}}r0, [q3+]
# CHECK-NOT: ldp
# CHECK: ldp8u{{[ \t]+}}r1, [q1+]
# CHECK-NOT: ldp
# CHECK: ldp8u{{[ \t]+}}r1, [q2+]
# CHECK-NOT: ldp
# CHECK: ldp8u{{[ \t]+}}r1, [q3+]
# CHECK-NOT: ldp
# CHECK: ldp8u{{[ \t]+}}r2, [q0+]
# CHECK-NOT: ldp
# CHECK: ldp8u{{[ \t]+}}r2, [q2+]
# CHECK-NOT: ldp
# CHECK: ldp8u{{[ \t]+}}r2, [q3+]
# CHECK-NOT: ldp
# CHECK: ldp8u{{[ \t]+}}r3, [q0+]
# CHECK-NOT: ldp
# CHECK: ldp8u{{[ \t]+}}r3, [q2+]
# CHECK-NOT: ldp
# CHECK: ldp8u{{[ \t]+}}r3, [q3+]
# CHECK-NOT: ldp
# CHECK: ldp8u{{[ \t]+}}r4, [q0+]
# CHECK-NOT: ldp
# CHECK: ldp8u{{[ \t]+}}r4, [q1+]
# CHECK-NOT: ldp
# CHECK: ldp8u{{[ \t]+}}r4, [q3+]
# CHECK-NOT: ldp
# CHECK: ldp8u{{[ \t]+}}r5, [q0+]
# CHECK-NOT: ldp
# CHECK: ldp8u{{[ \t]+}}r5, [q1+]
# CHECK-NOT: ldp
# CHECK: ldp8u{{[ \t]+}}r5, [q3+]
# CHECK-NOT: ldp
# CHECK: ldp8u{{[ \t]+}}r6, [q0+]
# CHECK-NOT: ldp
# CHECK: ldp8u{{[ \t]+}}r6, [q1+]
# CHECK-NOT: ldp
# CHECK: ldp8u{{[ \t]+}}r6, [q2+]
# CHECK-NOT: ldp
# CHECK: ldp8u{{[ \t]+}}r7, [q0+]
# CHECK-NOT: ldp
# CHECK: ldp8u{{[ \t]+}}r7, [q1+]
# CHECK-NOT: ldp
# CHECK: ldp8u{{[ \t]+}}r7, [q2+]
# CHECK-NOT: ldp
# CHECK: ldp16{{[ \t]+}}r0, [q1+]
# CHECK-NOT: ldp
# CHECK: ldp16{{[ \t]+}}r0, [q2+]
# CHECK-NOT: ldp
# CHECK: ldp16{{[ \t]+}}r0, [q3+]
# CHECK-NOT: ldp
# CHECK: ldp16{{[ \t]+}}r1, [q1+]
# CHECK-NOT: ldp
# CHECK: ldp16{{[ \t]+}}r1, [q2+]
# CHECK-NOT: ldp
# CHECK: ldp16{{[ \t]+}}r1, [q3+]
# CHECK-NOT: ldp
# CHECK: ldp16{{[ \t]+}}r2, [q0+]
# CHECK-NOT: ldp
# CHECK: ldp16{{[ \t]+}}r2, [q2+]
# CHECK-NOT: ldp
# CHECK: ldp16{{[ \t]+}}r2, [q3+]
# CHECK-NOT: ldp
# CHECK: ldp16{{[ \t]+}}r3, [q0+]
# CHECK-NOT: ldp
# CHECK: ldp16{{[ \t]+}}r3, [q2+]
# CHECK-NOT: ldp
# CHECK: ldp16{{[ \t]+}}r3, [q3+]
# CHECK-NOT: ldp
# CHECK: ldp16{{[ \t]+}}r4, [q0+]
# CHECK-NOT: ldp
# CHECK: ldp16{{[ \t]+}}r4, [q1+]
# CHECK-NOT: ldp
# CHECK: ldp16{{[ \t]+}}r4, [q3+]
# CHECK-NOT: ldp
# CHECK: ldp16{{[ \t]+}}r5, [q0+]
# CHECK-NOT: ldp
# CHECK: ldp16{{[ \t]+}}r5, [q1+]
# CHECK-NOT: ldp
# CHECK: ldp16{{[ \t]+}}r5, [q3+]
# CHECK-NOT: ldp
# CHECK: ldp16{{[ \t]+}}r6, [q0+]
# CHECK-NOT: ldp
# CHECK: ldp16{{[ \t]+}}r6, [q1+]
# CHECK-NOT: ldp
# CHECK: ldp16{{[ \t]+}}r6, [q2+]
# CHECK-NOT: ldp
# CHECK: ldp16{{[ \t]+}}r7, [q0+]
# CHECK-NOT: ldp
# CHECK: ldp16{{[ \t]+}}r7, [q1+]
# CHECK-NOT: ldp
# CHECK: ldp16{{[ \t]+}}r7, [q2+]
# CHECK-NOT: ldp
# CHECK: ldp24{{[ \t]+}}q0, [q1+]
# CHECK-NOT: ldp
# CHECK: ldp24{{[ \t]+}}q0, [q2+]
# CHECK-NOT: ldp
# CHECK: ldp24{{[ \t]+}}q0, [q3+]
# CHECK-NOT: ldp
# CHECK: ldp24{{[ \t]+}}q1, [q0+]
# CHECK-NOT: ldp
# CHECK: ldp24{{[ \t]+}}q1, [q2+]
# CHECK-NOT: ldp
# CHECK: ldp24{{[ \t]+}}q1, [q3+]
# CHECK-NOT: ldp
# CHECK: ldp24{{[ \t]+}}q2, [q0+]
# CHECK-NOT: ldp
# CHECK: ldp24{{[ \t]+}}q2, [q1+]
# CHECK-NOT: ldp
# CHECK: ldp24{{[ \t]+}}q2, [q3+]
# CHECK-NOT: ldp
# CHECK: ldp24{{[ \t]+}}q3, [q0+]
# CHECK-NOT: ldp
# CHECK: ldp24{{[ \t]+}}q3, [q1+]
# CHECK-NOT: ldp
# CHECK: ldp24{{[ \t]+}}q3, [q2+]
# CHECK-NOT: ldp
# CHECK: ldp32{{[ \t]+}}q0, [q1+]
# CHECK-NOT: ldp
# CHECK: ldp32{{[ \t]+}}q0, [q2+]
# CHECK-NOT: ldp
# CHECK: ldp32{{[ \t]+}}q0, [q3+]
# CHECK-NOT: ldp
# CHECK: ldp32{{[ \t]+}}q1, [q0+]
# CHECK-NOT: ldp
# CHECK: ldp32{{[ \t]+}}q1, [q2+]
# CHECK-NOT: ldp
# CHECK: ldp32{{[ \t]+}}q1, [q3+]
# CHECK-NOT: ldp
# CHECK: ldp32{{[ \t]+}}q2, [q0+]
# CHECK-NOT: ldp
# CHECK: ldp32{{[ \t]+}}q2, [q1+]
# CHECK-NOT: ldp
# CHECK: ldp32{{[ \t]+}}q2, [q3+]
# CHECK-NOT: ldp
# CHECK: ldp32{{[ \t]+}}q3, [q0+]
# CHECK-NOT: ldp
# CHECK: ldp32{{[ \t]+}}q3, [q1+]
# CHECK-NOT: ldp
# CHECK: ldp32{{[ \t]+}}q3, [q2+]
# CHECK-NOT: ldp
# TRUNC: warning: invalid instruction encoding
