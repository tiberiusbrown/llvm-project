# RUN: llvm-mc -triple=avm -filetype=obj %s -o %t.o
# RUN: ld.lld %t.o -o %t.default 2>&1 | FileCheck %s --check-prefix=OFF --allow-empty
# RUN: ld.lld --avm-stack-gap-limit=1 %t.o -o %t.limit-only 2>&1 | FileCheck %s --check-prefix=OFF --allow-empty
# RUN: ld.lld --avm-print-stack-gaps %t.o -o %t.gaps 2>&1 | FileCheck %s --check-prefix=ALL
# RUN: ld.lld --gc-sections --avm-print-stack-gaps %t.o -o %t.gc 2>&1 | FileCheck %s --check-prefix=ALL
# RUN: ld.lld --avm-print-stack-gaps --avm-stack-gap-limit=2 %t.o -o %t.two 2>&1 | FileCheck %s --check-prefix=TWO
# RUN: ld.lld --avm-print-stack-gaps --avm-stack-gap-limit=0 %t.o -o %t.zero 2>&1 | FileCheck %s --check-prefix=ZERO
# RUN: ld.lld --avm-print-stack-gaps --avm-stack-gap-limit=0 --avm-stack-gap-limit=1 %t.o -o %t.last 2>&1 | FileCheck %s --check-prefix=ONE
# RUN: ld.lld --avm-print-stack-usage --avm-print-stack-gaps -e complete %t.o -o %t.complete 2>&1 | FileCheck %s --check-prefix=COMPLETE
# RUN: ld.lld --avm-print-stack-gaps -e tailroot %t.o -o %t.tail 2>&1 | FileCheck %s --check-prefix=TAIL
# RUN: ld.lld --avm-print-stack-gaps -r %t.o -o %t.partial 2>&1 | FileCheck %s --check-prefix=OFF --allow-empty
# RUN: ld.lld --avm-print-stack-gaps %t.partial -o %t.relink 2>&1 | FileCheck %s --check-prefix=ALL
# RUN: not ld.lld --avm-stack-gap-limit=-1 %t.o -o %t.bad 2>&1 | FileCheck %s --check-prefix=NEGATIVE
# RUN: not ld.lld --avm-stack-gap-limit=abc %t.o -o %t.bad 2>&1 | FileCheck %s --check-prefix=INVALID
# RUN: not ld.lld --avm-stack-gap-limit=9223372036854775808 %t.o -o %t.bad 2>&1 | FileCheck %s --check-prefix=INVALID
# RUN: llvm-mc -triple=avm -filetype=obj -defsym MANY=1 %s -o %t.many.o
# RUN: ld.lld --avm-print-stack-gaps %t.many.o -o %t.many 2>&1 | FileCheck %s --check-prefix=DEFAULT-LIMIT
# RUN: llvm-mc -triple=avm -filetype=obj -defsym CALLBACK=1 %s -o %t.callback.o
# RUN: ld.lld -e complete --avm-print-stack-gaps %t.callback.o -o %t.callback 2>&1 | FileCheck %s --check-prefix=CALLBACK
# RUN: llvm-mc -triple=avm -filetype=obj -defsym HUGE=1 %s -o %t.huge.o
# RUN: not ld.lld --avm-print-stack-gaps --avm-stack-gap-limit=1 %t.huge.o -o %t.huge 2>&1 | FileCheck %s --check-prefix=OVER

# OFF-NOT: AVM stack
# ALL: AVM stack gap 1: shared: unresolved indirect call target
# ALL-NEXT: witness path (known costs only):
# ALL-NEXT:   _start: frame 10
# ALL-NEXT:     call: return address 3, outgoing arguments 2
# ALL-NEXT:   shared: frame 20
# ALL-NEXT:     call: return address 3, outgoing arguments 4
# ALL-NEXT:   <unknown target>
# ALL-NEXT:   known stack at gap: 42 bytes (lower bound)
# ALL: AVM stack gap 2: legacy: missing frame and call metadata
# ALL: legacy: frame 0 (unknown; lower bound)
# ALL: known stack at gap: 13 bytes (lower bound)
# ALL: AVM stack gap 3: opaque: compiler-marked incomplete (dynamic stack allocation or opaque inline assembly)
# ALL: known stack at gap: 43 bytes (lower bound)
# ALL: AVM stack gap 4: recursive: recursive call continuation
# ALL: recursive: frame 5
# ALL: recursive: frame 5
# ALL: <recursive continuation unknown>
# ALL: known stack at gap: 26 bytes (lower bound)
# ALL: AVM stack gap 5: no_calls: missing call metadata
# ALL: AVM stack gap 6: no_frame: missing frame metadata
# ALL: AVM stack gap 7: direct: unavailable direct call target
# ALL: AVM stack analysis gaps: 7; paths shown: 7; omitted: 0
# ALL-NOT: AVM stack gap 8
# TWO: AVM stack gap 1: shared:
# TWO: AVM stack gap 2: legacy:
# TWO-NOT: AVM stack gap 3
# TWO: AVM stack analysis gaps: 7; paths shown: 2; omitted: 5
# ZERO-NOT: AVM stack gap
# ZERO: AVM stack analysis gaps: 7; paths shown: 0; omitted: 7
# ONE: AVM stack gap 1: shared:
# ONE-NOT: AVM stack gap 2
# ONE: AVM stack analysis gaps: 7; paths shown: 1; omitted: 6
# COMPLETE: AVM maximum provable stack usage is 0 bytes; complete bound: yes
# COMPLETE-NOT: AVM stack gap
# COMPLETE: AVM stack analysis gaps: 0; paths shown: 0; omitted: 0
# TAIL: AVM stack gap 1: shared:
# TAIL: tailroot: frame 50
# TAIL: tail transfer: return address 0, outgoing arguments 0; caller frame released
# TAIL: shared: frame 20
# TAIL: known stack at gap: 27 bytes (lower bound)
# TAIL: AVM stack analysis gaps: 1; paths shown: 1; omitted: 0
# NEGATIVE: error: --avm-stack-gap-limit: expected a nonnegative integer
# INVALID: error: --avm-stack-gap-limit={{.*}}: number expected
# DEFAULT-LIMIT: AVM stack gap 10:
# DEFAULT-LIMIT-NOT: AVM stack gap 11:
# DEFAULT-LIMIT: AVM stack analysis gaps: 13; paths shown: 10; omitted: 3
# CALLBACK: AVM stack gap 1: callback: compiler-marked incomplete
# CALLBACK: callback entry: return address 3
# CALLBACK: callback: frame 9
# CALLBACK: known stack at gap: 12 bytes (lower bound)
# CALLBACK: AVM stack analysis gaps: 1; paths shown: 1; omitted: 0
# OVER: error: AVM maximum provable stack usage is {{[0-9]+}} bytes; limit is 256 bytes
# OVER: AVM stack gap 1: shared:
# OVER: AVM stack analysis gaps: 7; paths shown: 1; omitted: 6

.macro frame fn, size
 .section .stack_sizes,"o",@progbits,.text.\fn
 .3byte \fn
 .uleb128 \size
.endm
.macro marker fn, flags=8
 .section .avm.stackcalls,"o",@progbits,.text.\fn
 .3byte \fn
 .3byte 0
 .short 0
 .byte \flags
.endm
.macro edge fn, target, args=0, flags=0
 .section .avm.stackcalls,"o",@progbits,.text.\fn
 .3byte \fn
 .3byte \target
 .short \args
 .byte \flags
.endm
.macro function fn, size
 .section .text.\fn,"ax",@progbits
 .globl \fn
 .type \fn,@function
 \fn: ret
 frame \fn, \size
 marker \fn
.endm

.ifdef HUGE
function _start, 250
.else
function _start, 10
.endif
edge _start, shared, 2
edge _start, caller
edge _start, legacy
edge _start, opaque
edge _start, recursive
edge _start, no_calls
edge _start, no_frame
edge _start, direct

function shared, 20
edge shared, 0, 4, 1
.ifdef MANY
.rept 6
edge shared, 0, 0, 1
.endr
.endif
function caller, 10
# The shared gap is reported once, using the shortest route from _start.
edge caller, shared

.section .text.legacy,"ax",@progbits
.globl legacy
.type legacy,@function
legacy: ret

function opaque, 30
marker opaque, 12
function recursive, 5
edge recursive, recursive

.section .text.no_calls,"ax",@progbits
.globl no_calls
.type no_calls,@function
no_calls: ret
frame no_calls, 7

.section .text.no_frame,"ax",@progbits
.globl no_frame
.type no_frame,@function
no_frame: ret
marker no_frame

function direct, 0
edge direct, 0
function complete, 0
function tailroot, 50
edge tailroot, shared, 0, 2

# Retained without GC but unreachable; it must not add a reported gap.
.section .text.dead,"ax",@progbits
.type dead,@function
dead: ret

.ifdef CALLBACK
function callback, 9
marker callback, 12
.section .data,"aw",@progbits
.3byte callback
.endif
