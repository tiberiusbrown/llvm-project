# RUN: llvm-mc -triple=avm -filetype=obj %s -o %t
# RUN: llvm-objdump -d --no-show-raw-insn %t | FileCheck %s
# RUN: llvm-mc -triple=avm -show-encoding %s | FileCheck %s --check-prefix=ENC
# RUN: llvm-objdump -d --no-show-raw-insn %t | sed -n '/^[[:space:]]*[0-9a-f][0-9a-f]*:/s/.*:[[:space:]]*//p' > %t.s
# RUN: llvm-mc -triple=avm -filetype=obj %t.s -o %t.2
# RUN: llvm-objdump -s %t | sed '/file format/d' > %t.dump
# RUN: llvm-objdump -s %t.2 | sed '/file format/d' | diff %t.dump -
.text
fpoly 3
.float 1.0, -2.0, 0.5
nop
fpoly 2, 0x7fc12345, 0x80000000
nop
# CHECK: fpoly 3, 1.0, -2.0, 0.5
# CHECK-NEXT: nop
# CHECK: fpoly 2, 0x7fc12345, -0.0
# CHECK-NEXT: nop
# ENC: fpoly 3 {{.*}}encoding: [0xff,0x15,0x03]
# ENC: fpoly 2, 0x7fc12345, -0.0 {{.*}}encoding: [0xff,0x15,0x02,0x45,0x23,0xc1,0x7f,0x00,0x00,0x00,0x80]

fpoly 3, +1.0, -2.0, 5.0e-1
# ENC: fpoly 3, 1.0, -2.0, 0.5 {{.*}}encoding: [0xff,0x15,0x03,0x00,0x00,0x80,0x3f,0x00,0x00,0x00,0xc0,0x00,0x00,0x00,0x3f]
fpoly 3, 0x1.0p-149, -0.0, 3.4028234663852886e+38
# CHECK: fpoly 3, {{.*}}, -0.0, {{.*}}
fpoly 3, inf, -infinity, nan
# CHECK: fpoly 3, 0x7f800000, 0xff800000, 0x7fc00000
