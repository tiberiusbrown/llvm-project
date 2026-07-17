# RUN: llvm-mc -triple=avm-unknown-arduboyfx -show-encoding %s | FileCheck %s

# Compact-only forms accept the architectural names directly.
ldi16 r4, 123
ldi16 r5, 123
ldi16 r6, 123
ldi16 r7, 123
stsp8 [sp+5], r4
stsp8 [sp+5], r5
stsp8 [sp+5], r6
stsp8 [sp+5], r7
mulu8.w r4, r7
lsl16i r5, 8
zext8 r4
swap8 r7

# Dual-form operations resolve compactness from register IDs, not spelling.
mov r6, r5
mov r2, r5
add r4, r7
add r1, r7
and r6, r5
and r1, r5

# Compatibility aliases resolve to the same physical registers.
mov c2, c1

# Pair registers remain q0-q3.
jmpp q0

# CHECK: ldi16{{[ \t]+}}r4, 123{{.*}}encoding: [0xc4,0x7b,0x00]
# CHECK: ldi16{{[ \t]+}}r5, 123{{.*}}encoding: [0xc5,0x7b,0x00]
# CHECK: ldi16{{[ \t]+}}r6, 123{{.*}}encoding: [0xc6,0x7b,0x00]
# CHECK: ldi16{{[ \t]+}}r7, 123{{.*}}encoding: [0xc7,0x7b,0x00]
# CHECK: stsp8{{[ \t]+}}[sp+5], r4{{.*}}encoding: [0xf1,0x44]
# CHECK: stsp8{{[ \t]+}}[sp+5], r5{{.*}}encoding: [0xf1,0x45]
# CHECK: stsp8{{[ \t]+}}[sp+5], r6{{.*}}encoding: [0xf1,0x46]
# CHECK: stsp8{{[ \t]+}}[sp+5], r7{{.*}}encoding: [0xf1,0x47]
# CHECK: mulu8.w{{[ \t]+}}r4, r7{{.*}}encoding: [0xf3,0x13]
# CHECK: lsl16i{{[ \t]+}}r5, 8{{.*}}encoding: [0xfa,0x48]
# CHECK: zext8{{[ \t]+}}r4{{.*}}encoding: [0xf1,0x74]
# CHECK: swap8{{[ \t]+}}r7{{.*}}encoding: [0xf1,0x7f]
# CHECK: mov{{[ \t]+}}r6, r5{{.*}}encoding: [0x09]
# CHECK: mov{{[ \t]+}}r2, r5{{.*}}encoding: [0xf1,0x15]
# CHECK: add{{[ \t]+}}r4, r7{{.*}}encoding: [0x13]
# CHECK: add{{[ \t]+}}r1, r7{{.*}}encoding: [0xf2,0x0f]
# CHECK: and{{[ \t]+}}r6, r5{{.*}}encoding: [0x89]
# CHECK: and{{[ \t]+}}r1, r5{{.*}}encoding: [0xf9,0x34]
# CHECK: mov{{[ \t]+}}r6, r5{{.*}}encoding: [0x09]
# CHECK: jmpp{{[ \t]+}}q0{{.*}}encoding: [0xe4]
