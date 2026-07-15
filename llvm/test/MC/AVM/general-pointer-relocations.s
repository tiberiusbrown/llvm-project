# RUN: llvm-mc -triple=avm-unknown-arduboyfx -filetype=obj %s -o %t.o
# RUN: llvm-readobj --relocations %t.o | FileCheck %s

.text
ld8u r0, [r7]
ld16 r6, [r3+]
st8 [r2+], r2
st16 [r7], r0

# CHECK: Relocations [
# CHECK-NEXT: ]
