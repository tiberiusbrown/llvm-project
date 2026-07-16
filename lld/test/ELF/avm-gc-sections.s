# RUN: split-file %s %t
# RUN: llvm-mc -triple=avm -filetype=obj %t/start.s -o %t/start.o
# RUN: llvm-mc -triple=avm -filetype=obj %t/live.s -o %t/live.o
# RUN: llvm-mc -triple=avm -filetype=obj %t/dead.s -o %t/dead.o
# RUN: ld.lld -e _start --image-base=0 -Tdata=0x200 --gc-sections %t/start.o %t/live.o %t/dead.o -o %t/a.out
# RUN: llvm-readobj --sections %t/a.out | FileCheck %s --check-prefix=SECTIONS
# RUN: llvm-objdump -s -j .saved -j .data %t/a.out | FileCheck %s --check-prefix=DATA
# RUN: llvm-readobj --symbols %t/a.out | FileCheck %s --check-prefix=SYMBOLS

# SECTIONS: Name: .data
# SECTIONS: Flags [ (0x20000003)
# SECTIONS:   SHF_ALLOC
# SECTIONS:   SHF_AVM_DATASPACE
# SECTIONS:   SHF_WRITE
# SECTIONS-NOT: SHF_GNU_RETAIN
# SECTIONS: Size: 1
# SECTIONS: Name: .text
# SECTIONS: Name: .saved
# SECTIONS: Flags [ (0x20000003)
# SECTIONS: Size: 1

# DATA: Contents of section .data:
# DATA: 0200 56
# DATA: Contents of section .saved:
# DATA: 0408 12

# SYMBOLS: Name: live_saved
# SYMBOLS: Name: live_data
# SYMBOLS-NOT: Name: dead_saved
# SYMBOLS-NOT: Name: dead_data

#--- start.s
.text
.globl _start
_start:
  .short live_saved
  .short live_data

#--- live.s
.section .saved.live,"aw",@progbits
.globl live_saved
live_saved:
  .byte 0x12

.section .data.live,"aw",@progbits
.globl live_data
live_data:
  .byte 0x56

#--- dead.s
.section .saved.dead,"aw",@progbits
.globl dead_saved
dead_saved:
.byte 0x34

.section .data.dead,"aw",@progbits
.globl dead_data
dead_data:
  .byte 0x78
