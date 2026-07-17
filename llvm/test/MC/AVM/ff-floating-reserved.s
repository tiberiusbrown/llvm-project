# RUN: llvm-mc -triple=avm --disassemble < %S/Inputs/ff-floating-reserved.txt 2>&1 | FileCheck %s --check-prefix=RESERVED
# RUN: llvm-mc -triple=avm --disassemble < %S/Inputs/ff-floating-badspec.txt 2>&1 | FileCheck %s --check-prefix=BADSPEC

# RESERVED-COUNT-119: invalid instruction encoding
# BADSPEC: invalid instruction encoding
