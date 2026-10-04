# RUN: split-file %s %t
# RUN: llvm-mc -triple=avm -filetype=obj %t/main.s -o %t/main.o
# RUN: llvm-mc -triple=avm -filetype=obj %t/hook.s -o %t/hook.o
# RUN: llvm-ar cr %t/hooks.a %t/hook.o
# RUN: ld.lld --avm-print-stack-usage --avm-print-stack-gaps %t/main.o %t/hooks.a -o %t.out 2>&1 | FileCheck %s --check-prefix=ABSENT
# RUN: ld.lld --avm-print-stack-gaps -e unguarded %t/main.o %t/hooks.a -o %t.unguarded 2>&1 | FileCheck %s --check-prefix=UNGUARDED
# RUN: not ld.lld -u hook %t/main.o %t/hooks.a -o %t.present 2>&1 | FileCheck %s --check-prefix=PRESENT
# ABSENT: AVM maximum provable stack usage is 20 bytes; complete bound: yes
# ABSENT: AVM stack analysis gaps: 0
# UNGUARDED: unavailable direct call target
# PRESENT: error: AVM maximum provable stack usage is 263 bytes
# PRESENT: hook: frame 240

#--- main.s
.section .text._start,"ax",@progbits
.globl _start
_start: ret
.weak hook
.section .stack_sizes,"o",@progbits,.text._start
.3byte _start
.uleb128 20
.section .avm.stackcalls,"o",@progbits,.text._start
.3byte _start
.3byte 0
.short 0
.byte 8
.3byte _start
.3byte hook
.short 0
.byte 128

.section .text.unguarded,"ax",@progbits
.globl unguarded
unguarded: ret
.section .stack_sizes,"o",@progbits,.text.unguarded
.3byte unguarded
.uleb128 0
.section .avm.stackcalls,"o",@progbits,.text.unguarded
.3byte unguarded
.3byte 0
.short 0
.byte 8
.3byte unguarded
.3byte hook
.short 0
.byte 0

#--- hook.s
.section .text.hook,"ax",@progbits
.globl hook
.type hook,@function
hook: ret
.section .stack_sizes,"o",@progbits,.text.hook
.3byte hook
.uleb128 240
.section .avm.stackcalls,"o",@progbits,.text.hook
.3byte hook
.3byte 0
.short 0
.byte 8
