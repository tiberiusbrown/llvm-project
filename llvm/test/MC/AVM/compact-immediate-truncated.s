# RUN: llvm-mc -triple=avm --disassemble < %S/Inputs/compact-immediate-truncated-2.txt 2>&1 | FileCheck --check-prefix=TRUNC-2 %s
# RUN: llvm-mc -triple=avm --disassemble < %S/Inputs/compact-immediate-truncated-3.txt 2>&1 | FileCheck --check-prefix=TRUNC-3 %s

# TRUNC-2: warning: invalid instruction encoding
# TRUNC-3: warning: invalid instruction encoding
