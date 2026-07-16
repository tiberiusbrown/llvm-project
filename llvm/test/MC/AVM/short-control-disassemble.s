# RUN: llvm-mc -triple=avm --disassemble < %S/Inputs/short-control.txt 2>&1 | FileCheck %s
# RUN: llvm-mc -triple=avm --disassemble < %S/Inputs/short-control-invalid.txt 2>&1 | FileCheck --check-prefix=INVALID %s

# CHECK: breq -128
# CHECK: brne -1
# CHECK: brult 0
# CHECK: brslt 127
# CHECK: bruge -128
# CHECK: brsge 1
# CHECK: jmp8 1
# CHECK: call8 -2
# CHECK: adjsp -1
# CHECK: sys 0
# CHECK: sys 1
# CHECK: sys 2
# CHECK: sys 3

# INVALID-COUNT-9: warning: invalid instruction encoding
