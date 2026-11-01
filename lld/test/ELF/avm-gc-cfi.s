# RUN: split-file %s %t
# RUN: llvm-mc -triple=avm -filetype=obj %t/gc.s -o %t/gc.o
# RUN: ld.lld --gc-sections -e used -Ttext=0x10100 %t/gc.o -o %t/gc.out
# RUN: llvm-dwarfdump --debug-frame %t/gc.out 2>&1 | FileCheck %s --implicit-check-not=error:
# RUN: llvm-dwarfdump --verify %t/gc.out | FileCheck %s --check-prefix=VERIFY

# The discarded function's FDE keeps its place, but its range becomes empty and
# its instructions become no-ops. Its tombstone start address must not produce
# repeated set_loc rows.
# CHECK: FDE cie=00000000 pc=00010100...00010103
# CHECK:   DW_CFA_set_loc: 10101
# CHECK-NEXT:   DW_CFA_def_cfa_offset: +5
# CHECK-NEXT:   DW_CFA_offset: R3 -5
# CHECK:   DW_CFA_set_loc: 10102
# CHECK-NEXT:   DW_CFA_def_cfa_offset: +3
# CHECK-NEXT:   DW_CFA_restore: R3
# CHECK: FDE cie=00000000 pc=00000000...00000000
# CHECK-NEXT:   Format:       DWARF32
# CHECK-NEXT:   DW_CFA_nop:

# VERIFY: No errors.

#--- gc.s
	.cfi_sections .debug_frame
	.section .text.used,"ax",@progbits
	.globl used
used:
	.cfi_startproc
	push16 r3
	.cfi_def_cfa_offset 5
	.cfi_offset r3, -5
	pop16 r3
	.cfi_def_cfa_offset 3
	.cfi_restore r3
	ret
	.cfi_endproc

	.section .text.dead,"ax",@progbits
	.globl dead
dead:
	.cfi_startproc
	push16 r3
	.cfi_def_cfa_offset 5
	.cfi_offset r3, -5
	adjsp -4
	.cfi_def_cfa_offset 9
	adjsp 4
	.cfi_def_cfa_offset 5
	pop16 r3
	.cfi_def_cfa_offset 3
	.cfi_restore r3
	ret
	.cfi_endproc
