# RUN: not llvm-mc -triple=avm-unknown-arduboyfx -filetype=null %s 2>&1 | FileCheck %s

.cfi_startproc
.cfi_same_value pb
.cfi_same_value cb
.cfi_same_value flags
.cfi_endproc

mtpb r0
mfpb r0
ldpbi 1
ld8 r0, [pb:r1]
ldp8 r0, [pb:r1]

# CHECK-COUNT-3: error: unknown AVM register
# CHECK: error: unknown AVM instruction 'mtpb'
# CHECK: error: unknown AVM instruction 'mfpb'
# CHECK: error: unknown AVM instruction 'ldpbi'
# CHECK: error: unknown AVM register
# CHECK: error: unknown AVM instruction 'ldp8'
