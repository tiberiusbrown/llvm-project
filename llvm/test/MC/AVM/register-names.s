# RUN: llvm-mc -triple=avm-unknown-arduboyfx -filetype=null %s

# Canonical register spellings remain available to generic MC consumers.
.cfi_startproc
.cfi_same_value r0
.cfi_same_value r1
.cfi_same_value r2
.cfi_same_value r3
.cfi_same_value r4
.cfi_same_value r5
.cfi_same_value r6
.cfi_same_value r7
.cfi_same_value c0
.cfi_same_value c1
.cfi_same_value c2
.cfi_same_value c3
.cfi_same_value q0
.cfi_same_value q1
.cfi_same_value q2
.cfi_same_value q3
.cfi_same_value sp
.cfi_same_value pc
.cfi_same_value cc
.cfi_endproc
