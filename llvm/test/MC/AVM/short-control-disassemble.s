# RUN: llvm-mc -triple=avm --disassemble < %S/Inputs/short-control.txt 2>&1 | FileCheck %s
# RUN: llvm-mc -triple=avm --disassemble < %S/Inputs/short-control-invalid.txt 2>&1 | FileCheck --check-prefix=INVALID %s

# CHECK: breq8 -128
# CHECK: brne8 -1
# CHECK: brult8 0
# CHECK: brslt8 127
# CHECK: bruge8 -128
# CHECK: brsge8 1
# CHECK: jmp8 1
# CHECK: call8 -2
# CHECK: adjsp -1
# CHECK: sys debug_putc
# CHECK: sys debug_break
# CHECK: sys millis
# CHECK: sys millis32

# INVALID-COUNT-3: warning: invalid instruction encoding
