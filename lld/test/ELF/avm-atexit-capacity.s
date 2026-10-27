# RUN: split-file %s %t
# RUN: llvm-mc -triple=avm -filetype=obj %t/plain.s -o %t/plain.o
# RUN: llvm-mc -triple=avm -filetype=obj %t/mixed.s -o %t/mixed.o
# RUN: ld.lld --gc-sections %t/plain.o -o %t/default
# RUN: llvm-readobj --sections %t/default | FileCheck %s --check-prefix=DEFAULT
# RUN: ld.lld --gc-sections --avm-atexit-capacity=4 %t/plain.o -o %t/four
# RUN: llvm-readobj --sections %t/four | FileCheck %s --check-prefix=FOUR
# RUN: ld.lld --gc-sections %t/plain.o %t/mixed.o -o %t/mixed
# RUN: llvm-readobj --sections %t/mixed | FileCheck %s --check-prefix=MIXED
# RUN: not ld.lld --avm-atexit-capacity=0 %t/plain.o -o %t/bad 2>&1 | FileCheck %s --check-prefix=INVALID
# RUN: not ld.lld --avm-atexit-capacity=256 %t/plain.o -o %t/bad 2>&1 | FileCheck %s --check-prefix=INVALID
# RUN: not ld.lld --avm-atexit-capacity=oops %t/plain.o -o %t/bad 2>&1 | FileCheck %s --check-prefix=BAD-NUMBER
# DEFAULT: Name: .data
# DEFAULT: Size: 97
# FOUR: Name: .data
# FOUR: Size: 13
# MIXED: Name: .data
# MIXED: Size: 165
# INVALID: --avm-atexit-capacity: expected an integer from 1 to 255
# BAD-NUMBER: --avm-atexit-capacity

#--- plain.s
.section .text._start,"ax",@progbits
.globl _start
_start:
  call atexit
  ret
.section .text.atexit,"ax",@progbits
.globl atexit
.type atexit,@function
atexit:
  ldi16 r4, __avm_atexit_count
  ldi16 r4, __avm_local_dtor_slots_start
  ldi16 r4, __avm_termination_slots_end
  call __avm_run_local_dtors
  ret
.section .text.registry,"ax",@progbits
.globl __avm_run_local_dtors
.type __avm_run_local_dtors,@function
__avm_run_local_dtors:
  ldi16 r4, __avm_termination_next
  ldi16 r4, __avm_termination_phase
  ret
#--- mixed.s
.text
.type destructor,@function
destructor:
  ret
.section .fini_array,"a",@fini_array
.progptr destructor
