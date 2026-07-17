# RUN: llvm-mc -triple=avm --disassemble < %S/Inputs/ff-floating-valid.txt | FileCheck %s

# CHECK: fadd{{[ \t]+}}q0, q0
# CHECK: fsub{{[ \t]+}}q3, q3
# CHECK: fmul{{[ \t]+}}q2, q2
# CHECK: fdiv{{[ \t]+}}q3, q0
# CHECK: fmin{{[ \t]+}}q3, q1
# CHECK: fmax{{[ \t]+}}q3, q2
# CHECK: fneg{{[ \t]+}}q0
# CHECK: fabs{{[ \t]+}}q3
# CHECK: fsqrt{{[ \t]+}}q0
# CHECK: ftrunc{{[ \t]+}}q3
# CHECK: ffloor{{[ \t]+}}q0
# CHECK: fceil{{[ \t]+}}q3
# CHECK: fround{{[ \t]+}}q0
# CHECK: fround{{[ \t]+}}q3
# CHECK: s16tof{{[ \t]+}}q3, r7
# CHECK: ftou16{{[ \t]+}}r5, q2
# CHECK: s32tof{{[ \t]+}}q3, q3
# CHECK: ftou32{{[ \t]+}}q3, q0
# CHECK: fcmp{{[ \t]+}}r5, q2, q1
# CHECK: fclass{{[ \t]+}}r7, q3
