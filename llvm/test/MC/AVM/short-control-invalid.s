# RUN: not llvm-mc -triple=avm -show-encoding %s 2>&1 | FileCheck %s

breq -129
brne 128
brult 255
brslt 0xff
bruge -129
bruge 128
brsge -129
brsge 128
jmp8 128
call8 -129
adjsp 255
sys -1
sys 4
sys 255
sys symbol
breq
breq 1, 2
jmp
call 0
jmp8
call8 1, 2
adjsp
sys 0, 1

# CHECK-COUNT-11: error: {{(relative displacement|immediate) is out of signed 8-bit range}}
# CHECK-COUNT-3: error: invalid AVM version 1 service identifier
# CHECK: error: service expression must be fully resolvable
# CHECK: error: unknown token in expression
# CHECK: error: unexpected token after AVM instruction
