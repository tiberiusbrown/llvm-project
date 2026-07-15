# RUN: not llvm-mc -triple=avm-unknown-arduboyfx -filetype=null %s 2>&1 | FileCheck %s

.cfi_startproc
.cfi_same_value pb
.cfi_same_value cb
.cfi_same_value flags
.cfi_endproc

beq.s 0
bne.s 0
cpc16 r0, r1
adc r0, r1
sbc r0, r1
cpc r0, r1
mtpb r0
mfpb r0
ldpbi 1
ldi16 r0, 1
cmpi6 c0, 1
jmp16 0
jmpp q0
ld8 r0, [r1]
ldsp16 r0, [sp+0]
mov32 q0, q1
ret
nop
sys 0

# CHECK-COUNT-3: error: unknown AVM register
# CHECK: error: unknown AVM instruction 'beq.s'
# CHECK: error: unknown AVM instruction 'bne.s'
# CHECK: error: unknown AVM instruction 'cpc16'
# CHECK: error: unknown AVM instruction 'adc'
# CHECK: error: unknown AVM instruction 'sbc'
# CHECK: error: unknown AVM instruction 'cpc'
# CHECK: error: unknown AVM instruction 'mtpb'
# CHECK: error: unknown AVM instruction 'mfpb'
# CHECK: error: unknown AVM instruction 'ldpbi'
# CHECK: error: expected compact register c0-c3
# CHECK: error: unknown AVM instruction 'cmpi6'
# CHECK: error: unknown AVM instruction 'ld8'
# CHECK: error: unknown AVM instruction 'ldsp16'
# CHECK: error: unknown AVM instruction 'mov32'
