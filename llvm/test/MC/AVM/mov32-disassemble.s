# RUN: llvm-mc -triple=avm --disassemble < %S/Inputs/mov32-native.txt | FileCheck %s --check-prefix=NATIVE
# RUN: llvm-mc -triple=avm --disassemble < %S/Inputs/mov32-compact.txt | FileCheck %s --check-prefix=COMPACT

# NATIVE: mov32{{[ \t]+}}q0, q0
# NATIVE: mov32{{[ \t]+}}q0, q1
# NATIVE: mov32{{[ \t]+}}q0, q2
# NATIVE: mov32{{[ \t]+}}q0, q3
# NATIVE: mov32{{[ \t]+}}q1, q0
# NATIVE: mov32{{[ \t]+}}q1, q1
# NATIVE: mov32{{[ \t]+}}q1, q2
# NATIVE: mov32{{[ \t]+}}q1, q3
# NATIVE: mov32{{[ \t]+}}q2, q0
# NATIVE: mov32{{[ \t]+}}q2, q1
# NATIVE: mov32{{[ \t]+}}q3, q0
# NATIVE: mov32{{[ \t]+}}q3, q1

# COMPACT: nop
# COMPACT: mov{{[ \t]+}}r5, r5
# COMPACT: mov{{[ \t]+}}r4, r6
# COMPACT: mov{{[ \t]+}}r5, r7
# COMPACT: mov{{[ \t]+}}r6, r4
# COMPACT: mov{{[ \t]+}}r7, r5
# COMPACT: mov{{[ \t]+}}r6, r6
# COMPACT: mov{{[ \t]+}}r7, r7
