# RUN: not llvm-mc -triple=avm -show-encoding %s 2>&1 | FileCheck %s

ldm8u c0,[0]
ldm8u q0,[0]
ldm8u sp,[0]
ldm8u r0,0
ldm8u r0,[r1]
ldm8u r0,[sp+0]
ldm8u r0,[-1]
ldm8u r0,[65536]
ldm8u r0
ldm8u r0,[0],r1
stm8 [0],c0
stm8 r0,[0]
stm8 0,r0
stm8 [r1],r0
stm8 [sp+0],r0
stm8 [-1],r0
stm8 [65536],r0
stm8 [0]
stm8 [0],r0,r1
ldm16 c0,[0]
ldm16 r0,0
ldm16 r0,[r1]
ldm16 r0,[-1]
ldm16 r0,[65536]
ldm16 r0
stm16 [0],c0
stm16 r0,[0]
stm16 0,r0
stm16 [r1],r0
stm16 [-1],r0
stm16 [65536],r0
stm16 [0]

# CHECK: error:
