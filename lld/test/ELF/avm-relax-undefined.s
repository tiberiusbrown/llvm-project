# RUN: llvm-mc -triple=avm -filetype=obj %s -o %t.o
# RUN: llvm-readobj --relocations %t.o | FileCheck %s --check-prefix=RELOCS
# RUN: not ld.lld %t.o -o %t.out --error-limit=0 2>&1 | FileCheck %s --check-prefix=UNDEF

# RELOCS: 0x0 R_AVM_RELAX - 0x0
# RELOCS-NEXT: 0x1 R_AVM_FAR24 f 0x0
# UNDEF: ld.lld: error: undefined symbol: f
# UNDEF-NOT: ld.lld: error:
# UNDEF-NOT: Stack dump:

.text
.globl _start
_start:
  call f
