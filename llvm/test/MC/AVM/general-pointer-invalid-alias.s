# RUN: llvm-mc -triple=avm --disassemble < %S/Inputs/general-pointer-invalid-alias.txt 2>&1 | FileCheck %s

# Every structurally decodable prohibited load alias is rejected.
# CHECK-COUNT-15: warning: invalid instruction encoding
