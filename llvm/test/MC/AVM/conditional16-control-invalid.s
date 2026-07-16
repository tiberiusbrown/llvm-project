# RUN: not llvm-mc -triple=avm -show-encoding %s 2>&1 | FileCheck %s

breq16 -32769
brne16 32768
brult16 65535
bruge16 -32769
brslt16 32768
brsge16 65535

# CHECK-COUNT-6: error: relative displacement is out of signed 16-bit range
