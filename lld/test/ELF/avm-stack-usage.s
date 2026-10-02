# RUN: llvm-mc -triple=avm -filetype=obj %s -o %t.o
# RUN: ld.lld --avm-print-stack-usage %t.o -o %t.out 2>&1 | FileCheck %s --check-prefix=SAFE
# RUN: ld.lld --gc-sections --avm-print-stack-usage %t.o -o %t.gc 2>&1 | FileCheck %s --check-prefix=SAFE
# RUN: ld.lld -e B --avm-print-stack-usage %t.o -o %t.entry 2>&1 | FileCheck %s --check-prefix=ENTRY
# RUN: llvm-readobj --sections %t.out | FileCheck %s --check-prefix=STRIP
# RUN: ld.lld --emit-relocs %t.o -o %t.relocs
# RUN: llvm-readobj --sections %t.relocs | FileCheck %s --check-prefix=STRIP --implicit-check-not=.stack_sizes --implicit-check-not=.avm.stackcalls
# RUN: ld.lld -r %t.o -o %t.partial
# RUN: llvm-readobj --sections %t.partial | FileCheck %s --check-prefix=KEEP
# RUN: ld.lld --avm-print-stack-usage %t.partial -o %t.relink 2>&1 | FileCheck %s --check-prefix=SAFE
# RUN: llvm-mc -triple=avm -filetype=obj -defsym LEAF=155 %s -o %t.256.o
# RUN: ld.lld --avm-print-stack-usage %t.256.o -o %t.256 2>&1 | FileCheck %s --check-prefix=EXACT
# RUN: llvm-mc -triple=avm -filetype=obj -defsym LEAF=156 %s -o %t.257.o
# RUN: not ld.lld %t.257.o -o %t.257 2>&1 | FileCheck %s --check-prefix=OVER
# RUN: llvm-mc -triple=avm -filetype=obj -defsym INDIRECT=1 %s -o %t.indirect.o
# RUN: ld.lld --avm-print-stack-usage %t.indirect.o -o %t.indirect 2>&1 | FileCheck %s --check-prefix=UNKNOWN
# RUN: llvm-mc -triple=avm -filetype=obj -defsym INDIRECT=1 -defsym LEAF=156 %s -o %t.indirect-over.o
# RUN: not ld.lld %t.indirect-over.o -o %t.indirect-over 2>&1 | FileCheck %s --check-prefix=OVER
# RUN: llvm-mc -triple=avm -filetype=obj -defsym CYCLE=1 %s -o %t.cycle.o
# RUN: ld.lld --avm-print-stack-usage %t.cycle.o -o %t.cycle 2>&1 | FileCheck %s --check-prefix=UNKNOWN
# RUN: llvm-mc -triple=avm -filetype=obj -defsym CYCLE=1 -defsym LEAF=156 %s -o %t.cycle-over.o
# RUN: not ld.lld %t.cycle-over.o -o %t.cycle-over 2>&1 | FileCheck %s --check-prefix=CYCLEOVER
# RUN: llvm-mc -triple=avm -filetype=obj -defsym TAIL=1 -defsym LEAF=220 %s -o %t.tail.o
# RUN: ld.lld --avm-print-stack-usage %t.tail.o -o %t.tail 2>&1 | FileCheck %s --check-prefix=TAIL
# RUN: llvm-mc -triple=avm -filetype=obj -defsym ARGS=1 %s -o %t.args.o
# RUN: ld.lld --avm-print-stack-usage %t.args.o -o %t.args 2>&1 | FileCheck %s --check-prefix=ARGS
# RUN: llvm-mc -triple=avm -filetype=obj -defsym CALLARGS=214 %s -o %t.unknown-over.o
# RUN: not ld.lld %t.unknown-over.o -o %t.unknown-over 2>&1 | FileCheck %s --check-prefix=UNKNOWNOVER
# RUN: llvm-mc -triple=avm -filetype=obj -defsym BRANCH=1 %s -o %t.branch.o
# RUN: ld.lld --avm-print-stack-usage %t.branch.o -o %t.branch 2>&1 | FileCheck %s --check-prefix=SAFE
# RUN: llvm-mc -triple=avm -filetype=obj -defsym LEGACY=1 %s -o %t.legacy.o
# RUN: ld.lld --avm-print-stack-usage %t.legacy.o -o %t.legacy 2>&1 | FileCheck %s --check-prefix=UNKNOWN

# SAFE: AVM maximum provable stack usage is 161 bytes; complete bound: yes
# SAFE: _start: frame 40
# SAFE: call: return address 3, outgoing arguments 2
# SAFE: B: frame 50
# SAFE: call: return address 3, outgoing arguments 3
# SAFE: C: frame 60
# EXACT: AVM maximum provable stack usage is 256 bytes; complete bound: yes
# ENTRY: AVM maximum provable stack usage is 116 bytes; complete bound: yes
# ENTRY: B: frame 50
# ENTRY: C: frame 60
# OVER: error: AVM maximum provable stack usage is 257 bytes; limit is 256 bytes
# OVER: _start: frame 40
# OVER: call: return address 3, outgoing arguments 2
# OVER: B: frame 50
# OVER: call: return address 3, outgoing arguments 3
# OVER: C: frame 156
# UNKNOWN: complete bound: no
# CYCLEOVER: error: AVM maximum provable stack usage is {{[0-9]+}} bytes; limit is 256 bytes
# TAIL: AVM maximum provable stack usage is 220 bytes; complete bound: yes
# TAIL: tail transfer: return address 0, outgoing arguments 0
# ARGS: AVM maximum provable stack usage is 201 bytes
# ARGS: call: return address 3, outgoing arguments 158
# ARGS: small: frame 0
# UNKNOWNOVER: error: AVM maximum provable stack usage is 257 bytes; limit is 256 bytes
# UNKNOWNOVER: call: return address 3, outgoing arguments 214
# UNKNOWNOVER: <unknown target>
# STRIP-NOT: Name: .stack_sizes
# STRIP-NOT: Name: .avm.stackcalls
# STRIP: Name: .text
# STRIP-NOT: Name: .stack_sizes
# STRIP-NOT: Name: .avm.stackcalls
# KEEP: Name: .stack_sizes
# KEEP: Name: .avm.stackcalls

.ifndef LEAF
.set LEAF, 60
.endif
.macro frame fn, size, id
 .section .stack_sizes,"o",@progbits,\fn,unique,\id
 .3byte \fn
 .uleb128 \size
.ifndef LEGACY
 .section .avm.stackcalls,"o",@progbits,\fn,unique,\id
 .3byte \fn
 .3byte 0
 .short 0
 .byte 8
.endif
.endm
.macro edge fn, target, args, flags, id
 .section .avm.stackcalls,"o",@progbits,\fn,unique,\id
 .3byte \fn
 .3byte \target
 .short \args
 .byte \flags
.endm

.section .text.start,"ax",@progbits
.globl _start
.type _start,@function
_start:
.ifdef TAIL
 jmp B
.else
 call B
 call small
 ret
.endif
frame _start, 40, 1
.ifdef TAIL
edge _start, B, 0, 2, 1
.else
edge _start, B, 2, 0, 1
.endif
.ifdef ARGS
edge _start, small, 158, 0, 1
.else
edge _start, small, 0, 0, 1
.endif
.ifdef CALLARGS
edge _start, 0, CALLARGS, 1, 1
.endif

.section .text.B,"ax",@progbits
.globl B
.type B,@function
B:
.ifdef TAIL
 jmp C
.else
 call C
 ret
.endif
frame B, 50, 2
.ifdef TAIL
edge B, C, 0, 2, 2
.else
edge B, C, 3, 0, 2
.endif
.ifdef INDIRECT
edge B, 0, 10, 1, 2
.endif

.section .text.C,"ax",@progbits
.type C,@function
C:
.ifdef CYCLE
 call B
.endif
 ret
frame C, LEAF, 3
.ifdef CYCLE
edge C, B, 0, 0, 3
.endif

.section .text.small,"ax",@progbits
.type small,@function
small:
.ifdef BRANCH
 call small2
.endif
 ret
frame small, 0, 4
.ifdef BRANCH
edge small, small2, 0, 0, 4
.endif
.section .text.small2,"ax",@progbits
.type small2,@function
small2: ret
frame small2, 0, 7

# A live-but-unreachable unsafe path must not participate even without GC.
.section .text.dead,"ax",@progbits
.type dead,@function
dead: call dead2
 ret
frame dead, 240, 5
edge dead, dead2, 20, 0, 5
.section .text.dead2,"ax",@progbits
.type dead2,@function
dead2: ret
frame dead2, 50, 6
