# RUN: llvm-mc -triple=avm --disassemble \
# RUN:   < %S/Inputs/f7-post-memory-invalid-alias.txt 2>&1 | FileCheck %s

# CHECK-COUNT-8: warning: invalid instruction encoding
