# RUN: llvm-mc -triple=avm-unknown-arduboyfx -show-encoding %s | FileCheck %s --check-prefix=ENC
# RUN: llvm-mc -triple=avm-unknown-arduboyfx -filetype=obj %s -o %t.o
# RUN: llvm-objdump -d %t.o | FileCheck %s --check-prefix=DIS

.text
  breq  0
  brne  0
  brult 0
  bruge 0
  brslt 0
  brsge 0
  brule 0
  brugt 0

# ENC: breq 0{{.*}}encoding: [0xf5,0x00]
# ENC: brne 0{{.*}}encoding: [0xf6,0x00]
# ENC: brult 0{{.*}}encoding: [0xf7,0x00]
# ENC: bruge 0{{.*}}encoding: [0xf8,0x00]
# ENC: brslt 0{{.*}}encoding: [0xf9,0x00]
# ENC: brsge 0{{.*}}encoding: [0xfa,0x00]
# ENC: brule 0{{.*}}encoding: [0xfb,0x00]
# ENC: brugt 0{{.*}}encoding: [0xfc,0x00]

# DIS: f5 00{{.*}}breq
# DIS: f6 00{{.*}}brne
# DIS: f7 00{{.*}}brult
# DIS: f8 00{{.*}}bruge
# DIS: f9 00{{.*}}brslt
# DIS: fa 00{{.*}}brsge
# DIS: fb 00{{.*}}brule
# DIS: fc 00{{.*}}brugt
