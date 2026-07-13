# RUN: llvm-mc -triple=avm-unknown-arduboyfx -show-encoding %s | FileCheck %s --check-prefix=ENC
# RUN: llvm-mc -triple=avm-unknown-arduboyfx -filetype=obj %s -o %t.o
# RUN: llvm-objdump -d --no-show-raw-insn %t.o | FileCheck %s --check-prefix=DIS

and a, r0
or c0, r7
xor c0, c1
bic c0, c2
and c1, c0
or c2, c3
xor c1, c0
bic c3, c2

# ENC: and c0, r0{{.*}}encoding: [0x50]
# ENC: or c0, r7{{.*}}encoding: [0x5f]
# ENC: xor c0, r5{{.*}}encoding: [0x65]
# ENC: bic c0, r6{{.*}}encoding: [0x6e]
# ENC: and c1, c0{{.*}}encoding: [0xf4,0x34]
# ENC: or c2, c3{{.*}}encoding: [0xf4,0x4b]
# ENC: xor c1, c0{{.*}}encoding: [0xf4,0x54]
# ENC: bic c3, c2{{.*}}encoding: [0xf4,0x6e]

# DIS: and c0, r0
# DIS: or c0, r7
# DIS: xor c0, r5
# DIS: bic c0, r6
# DIS: and c1, c0
# DIS: or c2, c3
# DIS: xor c1, c0
# DIS: bic c3, c2
