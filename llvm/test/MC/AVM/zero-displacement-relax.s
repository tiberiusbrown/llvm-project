# RUN: llvm-mc -triple=avm -show-encoding %s | FileCheck %s --check-prefix=ASM
# RUN: llvm-mc -triple=avm -filetype=obj %s -o %t.o
# RUN: llvm-objdump -d %t.o | FileCheck %s --check-prefix=OBJ

# An explicit +0 is relaxed to the fastest legal non-displaced encoding.
ld8u r7, [r5+0]
ld8u r1, [r5+0]
ld16 r7, [r5+0]
ld16 r1, [r5+0]
st8 [r5+0], r7
st8 [r5+0], r1
st16 [r5+0], r7
st16 [r5+0], r1

# Lower address registers and nonzero displacements retain their form.
ld8u r7, [r1+0]
ld8u r7, [r5+1]

# ASM: ld8u{{.*}}r7, [r5]{{.*}}encoding: [0x4d]
# ASM-NEXT: ld8u{{.*}}r1, [r5]{{.*}}encoding: [0xf5,0x35]
# ASM-NEXT: ld16{{.*}}r7, [r5]{{.*}}encoding: [0x6d]
# ASM-NEXT: ld16{{.*}}r1, [r5]{{.*}}encoding: [0xf5,0x45]
# ASM-NEXT: st8{{.*}}[r5], r7{{.*}}encoding: [0x57]
# ASM-NEXT: st8{{.*}}[r5], r1{{.*}}encoding: [0xf3,0x05]
# ASM-NEXT: st16{{.*}}[r5], r7{{.*}}encoding: [0x77]
# ASM-NEXT: st16{{.*}}[r5], r1{{.*}}encoding: [0xf5,0x55]
# ASM: ld8u{{.*}}r7, [r1+0]{{.*}}encoding: [0xed,0xe2,0x20]
# ASM-NEXT: ld8u{{.*}}r7, [r5+1]{{.*}}encoding: [0xed,0xea,0x21]

# OBJ: 0: 4d{{ *}}ld8u{{.*}}r7, [r5]
# OBJ-NEXT: 1: f5 35{{ *}}ld8u{{.*}}r1, [r5]
# OBJ-NEXT: 3: 6d{{ *}}ld16{{.*}}r7, [r5]
# OBJ-NEXT: 4: f5 45{{ *}}ld16{{.*}}r1, [r5]
# OBJ-NEXT: 6: 57{{ *}}st8{{.*}}[r5], r7
# OBJ-NEXT: 7: f3 05{{ *}}st8{{.*}}[r5], r1
# OBJ-NEXT: 9: 77{{ *}}st16{{.*}}[r5], r7
# OBJ-NEXT: a: f5 55{{ *}}st16{{.*}}[r5], r1
# OBJ-NEXT: c: ed e2 20{{ *}}ld8u{{.*}}r7, [r1+0]
# OBJ-NEXT: f: ed ea 21{{ *}}ld8u{{.*}}r7, [r5+1]
