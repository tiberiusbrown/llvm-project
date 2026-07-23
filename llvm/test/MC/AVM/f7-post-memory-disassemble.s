# RUN: llvm-mc -triple=avm --disassemble < %S/Inputs/f7-post-memory-valid.txt \
# RUN:   | FileCheck %s --check-prefix=CASES
# RUN: llvm-mc -triple=avm --disassemble < %S/Inputs/f7-post-memory-valid.txt \
# RUN:   | FileCheck %s --check-prefix=COUNT

# CASES: ld8u r7, [r4+]
# CASES: ld8u r0, [r7+]
# CASES: ld16 r7, [r4+]
# CASES: ld16 r0, [r7+]
# CASES: st16 [r4+], r7
# CASES: st16 [r7+], r0
# COUNT-COUNT-88: r
