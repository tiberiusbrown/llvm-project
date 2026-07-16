# RUN: not llvm-mc -triple=avm -show-encoding %s 2>&1 | FileCheck %s

breq8 -129
brne8 128
brult8 255
brslt8 0xff
bruge8 -129
bruge8 128
brsge8 -129
brsge8 128
jmp8 128
call8 -129
adjsp 255
sys -1
sys 4
sys 255
sys symbol
sys debug_unknown
sys putc
sys break
breq8
breq8 1, 2
jmp
call 0
jmp8
call8 1, 2
adjsp
sys 0, 1

# CHECK-COUNT-11: error: {{(relative displacement|immediate) is out of signed 8-bit range}}
# CHECK-COUNT-3: error: invalid AVM version 1 service identifier
# CHECK: error: unknown AVM system function 'symbol'
# CHECK: error: unknown AVM system function 'debug_unknown'
# CHECK: error: unknown AVM system function 'putc'
# CHECK: error: unknown AVM system function 'break'
# CHECK: error: unknown token in expression
# CHECK: error: unexpected token after AVM instruction
