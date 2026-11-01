# RUN: llvm-mc -triple=avm-unknown-arduboyfx -filetype=obj %s -o %t.o
# RUN: llvm-readobj --sections %t.o | FileCheck %s --check-prefix=SECTIONS
# RUN: llvm-readobj --relocations %t.o | FileCheck %s --check-prefix=RELOC
# RUN: llvm-dwarfdump --debug-frame %t.o | FileCheck %s --check-prefix=DWARF

# Unwind information is emitted to .debug_frame, not the allocated .eh_frame.
# Each row address is a DW_CFA_set_loc operand with an R_AVM_DEBUG24
# relocation, so linker relaxation can move it. The CFA offsets follow the
# packed three-byte return address and the two-byte register slot.
#
# Both paths from the conditional branch and the first call reach .Lepilogue
# with CFA = SP + 9, so every row describes the same stack state on every path.
	.cfi_sections .debug_frame
	.text
	.globl cfi_rows
	.type cfi_rows,@function
cfi_rows:
	.cfi_startproc
	push16 r3
	.cfi_def_cfa_offset 5
	.cfi_offset r3, -5
	adjsp -4
	.cfi_def_cfa_offset 9
	call helper
	breq .Lepilogue
	call helper
	jmp .Lepilogue
.Lepilogue:
	adjsp 4
	.cfi_def_cfa_offset 5
	pop16 r3
	.cfi_def_cfa_offset 3
	.cfi_restore r3
	ret
	.cfi_endproc

# SECTIONS: Name: .debug_frame
# SECTIONS-NEXT: Type: SHT_PROGBITS
# SECTIONS-NEXT: Flags [ (0x0)
# SECTIONS-NOT: Name: .eh_frame

# The FDE start and each of the four row addresses are R_AVM_DEBUG24 relocations.
# RELOC: Section ({{.*}}) .rela.debug_frame {
# RELOC-NEXT: 0x18 R_AVM_32 .debug_frame 0x0
# RELOC-COUNT-5: R_AVM_DEBUG24 .text
# RELOC-NEXT: }

# DWARF: CIE
# DWARF:   Version: 4
# DWARF:   Address size: 3
# DWARF:   Data alignment factor: -1
# DWARF:   Return address column: 9
# DWARF:   DW_CFA_def_cfa: SP +3
# DWARF:   DW_CFA_offset: PC -3
# DWARF: FDE cie=00000000 pc=00000000...00000019
# DWARF-NEXT:   Format:       DWARF32
# DWARF-NEXT:   DW_CFA_set_loc: 1
# DWARF-NEXT:   DW_CFA_def_cfa_offset: +5
# DWARF-NEXT:   DW_CFA_offset: R3 -5
# DWARF-NEXT:   DW_CFA_set_loc: 3
# DWARF-NEXT:   DW_CFA_def_cfa_offset: +9
# DWARF-NEXT:   DW_CFA_set_loc: 17
# DWARF-NEXT:   DW_CFA_def_cfa_offset: +5
# DWARF-NEXT:   DW_CFA_set_loc: 18
# DWARF-NEXT:   DW_CFA_def_cfa_offset: +3
# DWARF-NEXT:   DW_CFA_restore: R3
# DWARF:   0x0: CFA=SP+3: PC=[CFA-3]
# DWARF-NEXT:   0x1: CFA=SP+5: R3=[CFA-5], PC=[CFA-3]
# DWARF-NEXT:   0x3: CFA=SP+9: R3=[CFA-5], PC=[CFA-3]
# DWARF-NEXT:   0x17: CFA=SP+5: R3=[CFA-5], PC=[CFA-3]
# DWARF-NEXT:   0x18: CFA=SP+3: PC=[CFA-3]
