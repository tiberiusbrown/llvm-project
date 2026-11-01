# RUN: split-file %s %t
# RUN: llvm-mc -triple=avm -filetype=obj %t/debug.s -o %t/debug.o
# RUN: ld.lld --no-relax -e relax_func -Ttext=0x10100 %t/debug.o -o %t/norelax.out
# RUN: ld.lld --relax -e relax_func -Ttext=0x10100 %t/debug.o -o %t/relax.out
# RUN: llvm-objdump -d %t/relax.out | FileCheck %s --check-prefix=CODE
# RUN: llvm-dwarfdump --verify %t/norelax.out | FileCheck %s --check-prefix=VERIFY
# RUN: llvm-dwarfdump --verify %t/relax.out | FileCheck %s --check-prefix=VERIFY
# RUN: llvm-dwarfdump --debug-info %t/norelax.out | FileCheck %s --check-prefix=NORELAX-INFO
# RUN: llvm-dwarfdump --debug-info %t/relax.out | FileCheck %s --check-prefix=RELAX-INFO
# RUN: llvm-dwarfdump --debug-ranges %t/norelax.out | FileCheck %s --check-prefix=NORELAX-RANGES
# RUN: llvm-dwarfdump --debug-ranges %t/relax.out | FileCheck %s --check-prefix=RELAX-RANGES

# The relaxed link shortens the three calls, the conditional branch, and the
# jump by 12 bytes in total. Every absolute debug reference must move by the
# relaxation that precedes it, not by the total in the containing function. The
# inlined range is a function symbol plus an addend that crosses three shortened
# instructions. Its end is the end-exclusive boundary after the last call.

# CODE: call8
# CODE: breq8
# CODE: jmp8

# VERIFY: No errors.

# The original layout has relax_func at 0x10101. Relaxation moves the lexical
# block to 0x10102..0x1010a and 0x1010c..0x1010f, and the inlined call to
# 0x10108..0x1010a.
# RELAX-INFO: DW_TAG_subprogram
# RELAX-INFO:   DW_AT_low_pc (0x010101)
# RELAX-INFO:   DW_AT_high_pc (0x010110)
# RELAX-INFO: DW_TAG_lexical_block
# RELAX-INFO:   DW_AT_ranges (0x00000000
# RELAX-INFO:      [0x010102, 0x01010a)
# RELAX-INFO:      [0x01010c, 0x01010f))
# RELAX-INFO: DW_TAG_inlined_subroutine
# RELAX-INFO:   DW_AT_low_pc (0x010108)
# RELAX-INFO:   DW_AT_high_pc (0x01010a)

# Without relaxation the same references keep their original addresses.
# NORELAX-INFO: DW_TAG_inlined_subroutine
# NORELAX-INFO:   DW_AT_low_pc (0x010110)
# NORELAX-INFO:   DW_AT_high_pc (0x010114)
# NORELAX-RANGES: .debug_ranges contents:
# NORELAX-RANGES: 00000000 010102 010114
# NORELAX-RANGES: 00000000 010116 01011b
# RELAX-RANGES: .debug_ranges contents:
# RELAX-RANGES: 00000000 ffffff 000000
# RELAX-RANGES: 00000000 010102 01010a
# RELAX-RANGES: 00000000 01010c 01010f

#--- debug.s
	.text
	.globl relax_helper
	.type relax_helper,@function
relax_helper:
	ret

	.globl relax_func
	.type relax_func,@function
relax_func:
	push16 r3
.Lrange0:
	call relax_helper
	call relax_helper
	breq .Lrange2
	call relax_helper
.Lrange2:
	adjsp 4
.Lrange3:
	jmp .Lrange4
.Lrange4:
	pop16 r3
.Lrange5:
	ret
.Lfunc_end:
	.size relax_func, .Lfunc_end-relax_func

	.section .debug_abbrev,"",@progbits
	.byte 1
	.byte 0x11
	.byte 1
	.byte 0x11, 0x01
	.byte 0x12, 0x01
	.byte 0, 0
	.byte 2
	.byte 0x2e
	.byte 1
	.byte 0x11, 0x01
	.byte 0x12, 0x01
	.byte 0, 0
	.byte 3
	.byte 0x0b
	.byte 1
	.byte 0x55, 0x17
	.byte 0, 0
	.byte 4
	.byte 0x1d
	.byte 0
	.byte 0x11, 0x01
	.byte 0x12, 0x01
	.byte 0, 0
	.byte 0

	.section .debug_info,"",@progbits
.Lcu_begin:
	.long .Lcu_end-.Lcu_start
.Lcu_start:
	.short 4
	.long 0
	.byte 3
	.byte 1
	.3byte relax_func
	.3byte .Lfunc_end
	.byte 2
	.3byte relax_func
	.3byte .Lfunc_end
	.byte 3
	.long .Ldebug_ranges0
	.byte 4
	.3byte relax_func+15
	.3byte .Lrange2
	.byte 0
	.byte 0
	.byte 0
.Lcu_end:

	.section .debug_ranges,"",@progbits
.Ldebug_ranges0:
	.3byte 0xffffff
	.3byte 0
	.3byte .Lrange0
	.3byte .Lrange2
	.3byte .Lrange3
	.3byte .Lrange5
	.3byte 0
	.3byte 0
