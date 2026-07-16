# RUN: not llvm-mc -triple=avm -show-encoding %s 2>&1 | FileCheck %s
+
add32 r0,q0
add32 q0,r0
add32 c0,q0
add32 q0,c0
add32 sp,q0
add32 q0,pc
sub32 r0,q0
sub32 q0,r0
sub32 q0,cc
add32 r0:r1,q0
sub32 q0,r2:r3
lsr32.1 r0
lsr32.1 c0
lsr32.1 sp
asr32.1 r0
asr32.1 q4
asr32.1 cc
bool c0
bool q0
bool sp
bool pc
bool cc
add32 q0,1
add32 q0,[q1]
sub32 symbol,q0
lsr32.1 1
asr32.1 [q0]
bool 1
bool symbol
bool [r0]
add32 q0
sub32 q0
lsr32.1
asr32.1
bool 1
lsr32 q0
asr32 q0
+
# CHECK-COUNT-38: error:
