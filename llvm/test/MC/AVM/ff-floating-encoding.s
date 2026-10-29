# RUN: llvm-mc -triple=avm -filetype=obj %s -o %t
# RUN: llvm-objdump -d --no-show-raw-insn %t | sed -n '/^[[:space:]]*[0-9a-f][0-9a-f]*:/s/.*:[[:space:]]*//p' > %t.s
# RUN: llvm-mc -triple=avm -filetype=obj %t.s -o %t.2
# RUN: llvm-objdump -s %t | sed '/file format/d' > %t.dump
# RUN: llvm-objdump -s %t.2 | sed '/file format/d' | diff %t.dump -
# RUN: llvm-mc -triple=avm --show-encoding %s | FileCheck %s

fadd
# CHECK: fadd {{.*}}encoding: [0xff,0x00]
fsub
# CHECK: fsub {{.*}}encoding: [0xff,0x01]
fmul
# CHECK: fmul {{.*}}encoding: [0xff,0x02]
fdiv
# CHECK: fdiv {{.*}}encoding: [0xff,0x03]
fneg q0
# CHECK: fneg q0 {{.*}}encoding: [0xff,0x04]
fneg q1
# CHECK: fneg q1 {{.*}}encoding: [0xff,0x05]
fneg q2
# CHECK: fneg q2 {{.*}}encoding: [0xff,0x06]
fneg q3
# CHECK: fneg q3 {{.*}}encoding: [0xff,0x07]
fabs q0
# CHECK: fabs q0 {{.*}}encoding: [0xff,0x08]
fabs q1
# CHECK: fabs q1 {{.*}}encoding: [0xff,0x09]
fabs q2
# CHECK: fabs q2 {{.*}}encoding: [0xff,0x0a]
fabs q3
# CHECK: fabs q3 {{.*}}encoding: [0xff,0x0b]
s16tof
# CHECK: s16tof {{.*}}encoding: [0xff,0x0c]
u16tof
# CHECK: u16tof {{.*}}encoding: [0xff,0x0d]
ftos16
# CHECK: ftos16 {{.*}}encoding: [0xff,0x0e]
ftou16
# CHECK: ftou16 {{.*}}encoding: [0xff,0x0f]
s32tof
# CHECK: s32tof {{.*}}encoding: [0xff,0x10]
u32tof
# CHECK: u32tof {{.*}}encoding: [0xff,0x11]
ftos32
# CHECK: ftos32 {{.*}}encoding: [0xff,0x12]
ftou32
# CHECK: ftou32 {{.*}}encoding: [0xff,0x13]
fcmp
# CHECK: fcmp {{.*}}encoding: [0xff,0x14]
fpoly 1, 1.0
# CHECK: fpoly 1, 1.0 {{.*}}encoding: [0xff,0x15,0x01,0x00,0x00,0x80,0x3f]
fsqrt
# CHECK: fsqrt {{.*}}encoding: [0xff,0x16]
ftrunc
# CHECK: ftrunc {{.*}}encoding: [0xff,0x17]
ffloor
# CHECK: ffloor {{.*}}encoding: [0xff,0x18]
fceil
# CHECK: fceil {{.*}}encoding: [0xff,0x19]
fround
# CHECK: fround {{.*}}encoding: [0xff,0x1a]
fmod
# CHECK: fmod {{.*}}encoding: [0xff,0x1b]
frempio2
# CHECK: frempio2 {{.*}}encoding: [0xff,0x1c]
fma
# CHECK: fma {{.*}}encoding: [0xff,0x1d]
