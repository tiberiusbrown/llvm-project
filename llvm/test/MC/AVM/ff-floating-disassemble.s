# RUN: llvm-mc -triple=avm --disassemble < %S/Inputs/ff-floating-valid.txt | FileCheck %s

# CHECK: fadd{{[ \t]*}}{{$}}
# CHECK: fsub{{[ \t]*}}{{$}}
# CHECK: fmul{{[ \t]*}}{{$}}
# CHECK: fdiv{{[ \t]*}}{{$}}
# CHECK: fneg{{[ \t]+}}q0{{[ \t]*}}{{$}}
# CHECK: fneg{{[ \t]+}}q1{{[ \t]*}}{{$}}
# CHECK: fneg{{[ \t]+}}q2{{[ \t]*}}{{$}}
# CHECK: fneg{{[ \t]+}}q3{{[ \t]*}}{{$}}
# CHECK: fabs{{[ \t]+}}q0{{[ \t]*}}{{$}}
# CHECK: fabs{{[ \t]+}}q1{{[ \t]*}}{{$}}
# CHECK: fabs{{[ \t]+}}q2{{[ \t]*}}{{$}}
# CHECK: fabs{{[ \t]+}}q3{{[ \t]*}}{{$}}
# CHECK: s16tof{{[ \t]*}}{{$}}
# CHECK: u16tof{{[ \t]*}}{{$}}
# CHECK: ftos16{{[ \t]*}}{{$}}
# CHECK: ftou16{{[ \t]*}}{{$}}
# CHECK: s32tof{{[ \t]*}}{{$}}
# CHECK: u32tof{{[ \t]*}}{{$}}
# CHECK: ftos32{{[ \t]*}}{{$}}
# CHECK: ftou32{{[ \t]*}}{{$}}
# CHECK: fcmp{{[ \t]*}}{{$}}
# CHECK: fclass{{[ \t]*}}{{$}}
# CHECK: fsqrt{{[ \t]*}}{{$}}
# CHECK: ftrunc{{[ \t]*}}{{$}}
# CHECK: ffloor{{[ \t]*}}{{$}}
# CHECK: fceil{{[ \t]*}}{{$}}
# CHECK: fround{{[ \t]*}}{{$}}
# CHECK: fmod{{[ \t]*}}{{$}}
# CHECK: frempio2{{[ \t]*}}{{$}}
# CHECK: fma{{[ \t]*}}{{$}}
