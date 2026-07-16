# RUN: llvm-mc -triple=avm --disassemble < %S/Inputs/fe-reserved.txt 2>&1 | FileCheck %s
# CHECK-COUNT-190: warning: invalid instruction encoding
# CHECK-NOT: mul16
# RUN: llvm-mc -triple=avm --disassemble < %S/Inputs/ff-and-truncated.txt 2>&1 | FileCheck %s --check-prefix=EDGE
# EDGE-COUNT-3: warning: invalid instruction encoding
# EDGE: ret
# EDGE: cset.eq r0
