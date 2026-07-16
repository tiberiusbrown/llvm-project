# RUN: llvm-mc -triple=avm --disassemble < %S/Inputs/fa-truncated.txt 2>&1 | FileCheck %s
# CHECK: warning: invalid instruction encoding
# CHECK: cset.eq
# RUN: llvm-mc -triple=avm --disassemble < %S/Inputs/fa-lone.txt 2>&1 | FileCheck %s --check-prefix=LONE
# LONE: warning: invalid instruction encoding
