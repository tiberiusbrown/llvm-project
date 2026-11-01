# RUN: split-file %s %t
# RUN: llvm-mc -triple=avm -filetype=obj %t/cfi.s -o %t/cfi.o
# RUN: ld.lld --no-relax -e cfi_func -Ttext=0x10100 %t/cfi.o -o %t/norelax.out
# RUN: ld.lld --relax -e cfi_func -Ttext=0x10100 %t/cfi.o -o %t/relax.out
# RUN: llvm-nm -n %t/norelax.out | FileCheck %s --check-prefix=NORELAX-SYM
# RUN: llvm-nm -n %t/relax.out | FileCheck %s --check-prefix=RELAX-SYM
# RUN: llvm-objdump -s -j .text %t/norelax.out | FileCheck %s --check-prefix=NORELAX-BYTES
# RUN: llvm-objdump -s -j .text %t/relax.out | FileCheck %s --check-prefix=RELAX-BYTES
# RUN: llvm-dwarfdump --debug-frame %t/norelax.out | FileCheck %s --check-prefix=NORELAX-DWARF
# RUN: llvm-dwarfdump --debug-frame %t/relax.out | FileCheck %s --check-prefix=RELAX-DWARF
# RUN: llvm-dwarfdump --verify %t/relax.out | FileCheck %s --check-prefix=VERIFY

# The relaxed link shrinks the call, branch, and jump sites by 12 bytes in
# total. Every .debug_frame row must follow its label, and each FDE must end at
# the final end of its function.

# NORELAX-SYM: 00010100 T cfi_helper
# NORELAX-SYM-NEXT: 00010101 T cfi_func
# NORELAX-SYM-NEXT: 00010102 t cfi_row1
# NORELAX-SYM-NEXT: 00010104 t cfi_row2
# NORELAX-SYM-NEXT: 00010116 t cfi_done
# NORELAX-SYM-NEXT: 00010118 t cfi_row3
# NORELAX-SYM-NEXT: 00010119 t cfi_row4
# NORELAX-SYM-NEXT: 0001011a t cfi_func_end
# NORELAX-SYM-NEXT: 0001011a T cfi_leaf

# RELAX-SYM: 00010100 T cfi_helper
# RELAX-SYM-NEXT: 00010101 T cfi_func
# RELAX-SYM-NEXT: 00010102 t cfi_row1
# RELAX-SYM-NEXT: 00010104 t cfi_row2
# RELAX-SYM-NEXT: 0001010c t cfi_done
# RELAX-SYM-NEXT: 0001010e t cfi_row3
# RELAX-SYM-NEXT: 0001010f t cfi_row4
# RELAX-SYM-NEXT: 00010110 t cfi_func_end
# RELAX-SYM-NEXT: 00010110 T cfi_leaf

# The call at 0x10104 becomes d5 fa, the branch at 0x10106 becomes d0 04, the
# second call becomes d5 f6, the jump becomes d4 00, and the leaf jump becomes d4 ee.
# NORELAX-BYTES: 10100 efb3d6fc e3000101 d104e216 0101e300
# NORELAX-BYTES-NEXT: 10110 0101e216 0101d604 bbefe200 0101
# RELAX-BYTES: 10100 efb3d6fc d5fad004 d5f6d400 d604bbef
# RELAX-BYTES-NEXT: 10110 d4ee

# NORELAX-DWARF: FDE cie=00000000 pc=00010101...0001011a
# NORELAX-DWARF:   DW_CFA_set_loc: 10102
# NORELAX-DWARF-NEXT:   DW_CFA_def_cfa_offset: +5
# NORELAX-DWARF-NEXT:   DW_CFA_offset: R3 -5
# NORELAX-DWARF:   DW_CFA_set_loc: 10104
# NORELAX-DWARF-NEXT:   DW_CFA_def_cfa_offset: +9
# NORELAX-DWARF:   DW_CFA_set_loc: 10118
# NORELAX-DWARF-NEXT:   DW_CFA_def_cfa_offset: +5
# NORELAX-DWARF:   DW_CFA_set_loc: 10119
# NORELAX-DWARF-NEXT:   DW_CFA_def_cfa_offset: +3
# NORELAX-DWARF-NEXT:   DW_CFA_restore: R3
# NORELAX-DWARF:   0x10101: CFA=SP+3: PC=[CFA-3]
# NORELAX-DWARF:   0x10102: CFA=SP+5: R3=[CFA-5], PC=[CFA-3]
# NORELAX-DWARF:   0x10104: CFA=SP+9: R3=[CFA-5], PC=[CFA-3]
# NORELAX-DWARF:   0x10118: CFA=SP+5: R3=[CFA-5], PC=[CFA-3]
# NORELAX-DWARF:   0x10119: CFA=SP+3: PC=[CFA-3]
# NORELAX-DWARF: FDE cie=00000000 pc=0001011a...0001011e
# NORELAX-DWARF:   0x1011a: CFA=SP+3: PC=[CFA-3]

# RELAX-DWARF: FDE cie=00000000 pc=00010101...00010110
# RELAX-DWARF-NEXT:   Format:       DWARF32
# RELAX-DWARF-NEXT:   DW_CFA_set_loc: 10102
# RELAX-DWARF-NEXT:   DW_CFA_def_cfa_offset: +5
# RELAX-DWARF-NEXT:   DW_CFA_offset: R3 -5
# RELAX-DWARF:   DW_CFA_set_loc: 10104
# RELAX-DWARF-NEXT:   DW_CFA_def_cfa_offset: +9
# RELAX-DWARF:   DW_CFA_set_loc: 1010e
# RELAX-DWARF-NEXT:   DW_CFA_def_cfa_offset: +5
# RELAX-DWARF:   DW_CFA_set_loc: 1010f
# RELAX-DWARF-NEXT:   DW_CFA_def_cfa_offset: +3
# RELAX-DWARF-NEXT:   DW_CFA_restore: R3
# RELAX-DWARF:   0x10101: CFA=SP+3: PC=[CFA-3]
# RELAX-DWARF:   0x10102: CFA=SP+5: R3=[CFA-5], PC=[CFA-3]
# RELAX-DWARF:   0x10104: CFA=SP+9: R3=[CFA-5], PC=[CFA-3]
# RELAX-DWARF:   0x1010e: CFA=SP+5: R3=[CFA-5], PC=[CFA-3]
# RELAX-DWARF:   0x1010f: CFA=SP+3: PC=[CFA-3]
# RELAX-DWARF: FDE cie=00000000 pc=00010110...00010112
# RELAX-DWARF:   0x10110: CFA=SP+3: PC=[CFA-3]

# VERIFY: No errors.

#--- cfi.s
	.cfi_sections .debug_frame
	.text
	.globl cfi_helper
	.type cfi_helper,@function
cfi_helper:
	ret

	.globl cfi_func
	.type cfi_func,@function
cfi_func:
	.cfi_startproc
	push16 r3
cfi_row1:
	.cfi_def_cfa_offset 5
	.cfi_offset r3, -5
	adjsp -4
cfi_row2:
	.cfi_def_cfa_offset 9
	call cfi_helper
	breq cfi_done
	call cfi_helper
	jmp cfi_done
cfi_done:
	adjsp 4
cfi_row3:
	.cfi_def_cfa_offset 5
	pop16 r3
cfi_row4:
	.cfi_def_cfa_offset 3
	.cfi_restore r3
	ret
cfi_func_end:
	.cfi_endproc

	.globl cfi_leaf
	.type cfi_leaf,@function
cfi_leaf:
	.cfi_startproc
	jmp cfi_helper
	.cfi_endproc
