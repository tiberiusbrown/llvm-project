# RUN: split-file %s %t
# RUN: llvm-mc -triple=avm -filetype=obj %t/calls.s -o %t/calls.o
# RUN: not ld.lld %t/calls.o -o %t/calls 2>&1 | FileCheck %s --check-prefix=CALLS
# RUN: llvm-mc -triple=avm -filetype=obj %t/frame.s -o %t/frame.o
# RUN: not ld.lld %t/frame.o -o %t/frame 2>&1 | FileCheck %s --check-prefix=FRAME
# RUN: llvm-mc -triple=avm -filetype=obj %t/flags.s -o %t/flags.o
# RUN: not ld.lld %t/flags.o -o %t/flags 2>&1 | FileCheck %s --check-prefix=FLAGS
# CALLS: truncated .avm.stackcalls record
# FRAME: invalid AVM stack size
# FLAGS: unknown AVM stack call flags

#--- calls.s
.globl _start
.text
_start: ret
.section .avm.stackcalls,"o",@progbits,.text
.3byte _start
.byte 0

#--- frame.s
.globl _start
.text
_start: ret
.section .stack_sizes,"o",@progbits,.text
.3byte _start
.byte 128

#--- flags.s
.globl _start
.text
_start: ret
.section .avm.stackcalls,"o",@progbits,.text
.3byte _start
.3byte 0
.short 0
.byte 128
