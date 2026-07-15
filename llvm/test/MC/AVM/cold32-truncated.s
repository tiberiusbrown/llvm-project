# RUN: llvm-mc -triple=avm --disassemble < %S/Inputs/cold32-truncated-f0.txt 2>&1 | FileCheck --check-prefix=TRUNC %s
# RUN: llvm-mc -triple=avm --disassemble < %S/Inputs/cold32-truncated-69.txt 2>&1 | FileCheck --check-prefix=TRUNC %s
# RUN: llvm-mc -triple=avm --disassemble < %S/Inputs/cold32-truncated-6a.txt 2>&1 | FileCheck --check-prefix=TRUNC %s
# RUN: llvm-mc -triple=avm --disassemble < %S/Inputs/cold32-truncated-6b.txt 2>&1 | FileCheck --check-prefix=TRUNC %s
# TRUNC: warning: invalid instruction encoding
