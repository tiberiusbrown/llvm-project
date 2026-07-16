# RUN: llvm-mc -triple=avm --disassemble < %S/Inputs/f8-boundaries.txt 2>&1 | FileCheck %s

# CHECK: warning: invalid instruction encoding
# CHECK: cset.eq	r0
# CHECK: cset.sge	r7
