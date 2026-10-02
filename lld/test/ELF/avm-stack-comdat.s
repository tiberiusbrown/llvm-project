# RUN: split-file %s %t
# RUN: llvm-mc -triple=avm -filetype=obj %t/entry.s -o %t/entry.o
# RUN: llvm-mc -triple=avm -filetype=obj %t/a.s -o %t/a.o
# RUN: llvm-mc -triple=avm -filetype=obj %t/b.s -o %t/b.o
# RUN: ld.lld --gc-sections --avm-print-stack-usage %t/entry.o %t/a.o %t/b.o -o %t/out 2>&1 | FileCheck %s --check-prefix=SAFE
# RUN: not ld.lld --gc-sections %t/entry.o %t/b.o %t/a.o -o %t/over 2>&1 | FileCheck %s --check-prefix=OVER
# SAFE: AVM maximum provable stack usage is 143 bytes; complete bound: yes
# SAFE: foo: frame 100
# OVER: error: AVM maximum provable stack usage is 283 bytes; limit is 256 bytes
# OVER: foo: frame 240

#--- entry.s
.text
.globl _start
_start: call foo
 ret
.section .stack_sizes,"o",@progbits,.text
.3byte _start
.uleb128 40
.section .avm.stackcalls,"o",@progbits,.text
.3byte _start
.3byte 0
.short 0
.byte 8
.3byte _start
.3byte foo
.short 0
.byte 0

#--- a.s
.section .text.foo,"axG",@progbits,foo,comdat
.weak foo
.type foo,@function
foo: ret
.section .stack_sizes,"oG",@progbits,.text.foo,foo,comdat
.3byte foo
.uleb128 100
.section .avm.stackcalls,"oG",@progbits,.text.foo,foo,comdat
.3byte foo
.3byte 0
.short 0
.byte 8

#--- b.s
.section .text.foo,"axG",@progbits,foo,comdat
.weak foo
.type foo,@function
foo: ret
.section .stack_sizes,"oG",@progbits,.text.foo,foo,comdat
.3byte foo
.uleb128 240
.section .avm.stackcalls,"oG",@progbits,.text.foo,foo,comdat
.3byte foo
.3byte 0
.short 0
.byte 8
