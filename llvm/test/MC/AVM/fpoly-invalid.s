# RUN: not llvm-mc -triple=avm -filetype=obj %s -o /dev/null 2>&1 | FileCheck %s
fpoly 0
# CHECK: fpoly coefficient count must be 1 through 255
fpoly 256
# CHECK: out of unsigned 8-bit range
fpoly -1
# CHECK: out of unsigned 8-bit range
fpoly 2, 0x3f800000
# CHECK: fpoly coefficient count does not match inline data
fpoly 1, 0x100000000
# CHECK: fpoly coefficient bits must fit 32 bits
