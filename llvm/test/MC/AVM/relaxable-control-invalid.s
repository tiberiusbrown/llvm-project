# RUN: not llvm-mc -triple=avm -show-encoding %s 2>&1 | FileCheck %s

jmp
jmp target, other
call
call target, other
br.eq
br.eq target, other
br.ne
br.ult
br.uge
br.slt
br.sge

jmp 0
call 0
br.eq 0
br.ne -2
br.ult 12
br.uge 12
br.slt 1
br.sge 1
jmp r0
call r0
br.eq r0
br.ne q0

j target
jump target
buge target
bsge target
bruge8 target
brsge8 target
br target
br. target
br.eq. target
br.ule target
br.ugt target

# CHECK-COUNT-8: error: relaxable '{{(jmp|call|br\.(eq|ne|ult|uge|slt|sge))}}' requires a symbolic target
# CHECK-COUNT-3: error: expected relocatable symbolic program target
# CHECK: error: unknown AVM instruction 'j'
# CHECK: error: unknown AVM instruction 'jump'
# CHECK: error: unknown AVM instruction 'buge'
# CHECK: error: unknown AVM instruction 'bsge'
# CHECK: error: unknown AVM instruction 'bruge8'
# CHECK: error: unknown AVM instruction 'brsge8'
# CHECK: error: unknown AVM instruction 'br'
# CHECK: error: unknown AVM instruction 'br.'
# CHECK: error: unknown AVM instruction 'br.eq.'
# CHECK: error: unknown AVM instruction 'br.ule'
# CHECK: error: unknown AVM instruction 'br.ugt'
