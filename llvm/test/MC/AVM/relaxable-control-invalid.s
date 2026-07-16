# RUN: not llvm-mc -triple=avm -show-encoding %s 2>&1 | FileCheck %s

jmp
jmp target, other
call
call target, other
breq
breq target, other
brne
brult
bruge
brslt
brsge

jmp 0
call 0
breq 0
brne -2
brult 12
bruge 12
brslt 1
brsge 1
jmp r0
call r0
breq r0
brne q0

j target
jump target
buge target
bsge target
br.eq target
br.ne target
br.ult target
br.uge target
br.slt target
br.sge target
br target
br. target
br.ule target
br.ugt target

# CHECK-COUNT-8: error: relaxable '{{(jmp|call|br(eq|ne|ult|uge|slt|sge))}}' requires a symbolic target
# CHECK-COUNT-3: error: expected relocatable symbolic program target
# CHECK: error: unknown AVM instruction 'j'
# CHECK: error: unknown AVM instruction 'jump'
# CHECK: error: unknown AVM instruction 'buge'
# CHECK: error: unknown AVM instruction 'bsge'
# CHECK: error: unknown AVM instruction 'br.eq'
# CHECK: error: unknown AVM instruction 'br.ne'
# CHECK: error: unknown AVM instruction 'br.ult'
# CHECK: error: unknown AVM instruction 'br.uge'
# CHECK: error: unknown AVM instruction 'br.slt'
# CHECK: error: unknown AVM instruction 'br.sge'
# CHECK: error: unknown AVM instruction 'br'
# CHECK: error: unknown AVM instruction 'br.'
# CHECK: error: unknown AVM instruction 'br.ule'
# CHECK: error: unknown AVM instruction 'br.ugt'
