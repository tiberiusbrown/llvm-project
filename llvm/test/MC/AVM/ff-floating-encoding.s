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
fclass
# CHECK: fclass {{.*}}encoding: [0xff,0x15]
