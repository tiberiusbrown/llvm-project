# RUN: split-file %s %t
# RUN: llvm-mc -triple=avm -filetype=obj %t/start.s -o %t/start.o
# RUN: llvm-mc -triple=avm -filetype=obj %t/plain.s -o %t/plain.o
# RUN: ld.lld %t/start.o -o %t/empty
# RUN: llvm-readobj --symbols %t/empty | FileCheck %s --check-prefix=EMPTY
# RUN: llvm-mc -triple=avm -filetype=obj --defsym=N=1 %t/data.s -o %t/d1.o
# RUN: ld.lld %t/start.o %t/d1.o -o %t/a1
# RUN: llvm-readobj --symbols --sections %t/a1 | FileCheck %s --check-prefix=D1
# RUN: llvm-mc -triple=avm -filetype=obj --defsym=N=255 %t/data.s -o %t/d255.o
# RUN: ld.lld %t/start.o %t/d255.o -o %t/a255
# RUN: llvm-readobj --symbols --sections %t/a255 | FileCheck %s --check-prefix=D255
# RUN: llvm-mc -triple=avm -filetype=obj --defsym=N=256 %t/data.s -o %t/d256.o
# RUN: ld.lld %t/start.o %t/d256.o -o %t/a256
# RUN: llvm-readobj --symbols --sections %t/a256 | FileCheck %s --check-prefix=D256
# RUN: llvm-mc -triple=avm -filetype=obj --defsym=N=1024 %t/data.s -o %t/d1024.o
# RUN: ld.lld %t/start.o %t/d1024.o -o %t/a1024
# RUN: llvm-readobj --symbols --sections %t/a1024 | FileCheck %s --check-prefix=D1024
# RUN: llvm-mc -triple=avm -filetype=obj --defsym=N=1025 %t/data.s -o %t/d1025.o
# RUN: llvm-mc -triple=avm -filetype=obj %t/saved.s -o %t/saved.o
# RUN: ld.lld %t/start.o %t/d1.o %t/saved.o -o %t/saved
# RUN: llvm-readobj --symbols %t/saved | FileCheck %s --check-prefix=SAVED
# RUN: ld.lld --gc-sections %t/start.o %t/d256.o -o %t/gc
# RUN: llvm-readobj --symbols %t/gc | FileCheck %s --check-prefix=EMPTY
# RUN: ld.lld %t/plain.o %t/d256.o -o %t/plain
# RUN: llvm-readobj --sections %t/plain | FileCheck %s --check-prefix=SIZE
# RUN: llvm-readobj --sections %t/a256 | FileCheck %s --check-prefix=SIZE
# RUN: llvm-objdump -s -j .text %t/a256 | FileCheck %s --check-prefix=RELOC
# RUN: not ld.lld %t/start.o %t/d1025.o -o %t/overflow 2>&1 | FileCheck %s --check-prefix=OVERFLOW
# RUN: llvm-mc -triple=avm -filetype=obj %t/termination.s -o %t/termination.o
# RUN: ld.lld --avm-atexit-capacity=4 %t/start.o %t/termination.o %t/d1.o -o %t/termination
# RUN: llvm-readobj --symbols %t/termination | FileCheck %s --check-prefix=TERM
# EMPTY: Name: __avm_heap_start
# EMPTY-NEXT: Value: 0x100
# EMPTY-NEXT: Size: 0
# EMPTY: Other [ (0x2)
# EMPTY-NEXT: STV_HIDDEN
# EMPTY: Section: Absolute
# D1: Name: __avm_heap_start
# D1-NEXT: Value: 0x101
# D255: Name: __avm_heap_start
# D255-NEXT: Value: 0x1FF
# D256: Name: __avm_heap_start
# D256-NEXT: Value: 0x200
# D1024: Name: __avm_heap_start
# D1024-NEXT: Value: 0x500
# SAVED: Name: __avm_heap_start
# SAVED-NEXT: Value: 0x102
# SIZE: Name: .data
# SIZE: Size: 256
# RELOC: 0200 0002ef00
# OVERFLOW: AVM static storage exceeds the 1024-byte limit
# TERM: Name: __avm_heap_start
# TERM-NEXT: Value: 0x10E

#--- start.s
.text
.globl _start
_start:
  .short __avm_heap_start
  ret
#--- plain.s
.text
.globl _start
_start:
  .short 0
  ret
#--- data.s
.section .data.object,"aw",@progbits
.zero N
#--- saved.s
.section .saved.object,"aw",@progbits
.byte 1
#--- termination.s
.text
.globl atexit
.type atexit,@function
atexit:
  ldi16 r4, __avm_atexit_count
  ret
