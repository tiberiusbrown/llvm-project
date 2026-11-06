; RUN: llvm-mc -triple=avm -filetype=obj %s -o %t.o
; RUN: llvm-dwarfdump --debug-frame %t.o | FileCheck %s
; RUN: llvm-dwarfdump --verify %t.o

; The ELF container uses four-byte addresses, but the CIE specifies the
; three-byte code addresses used by DW_CFA_set_loc. Lazy CFI decoding must
; retain the CIE's address size rather than using the ELF default.
; CHECK: Address size:          3
; CHECK: DW_CFA_set_loc: 1
; CHECK-NEXT: DW_CFA_def_cfa_offset: +5
; CHECK: DW_CFA_set_loc: 2
; CHECK-NEXT: DW_CFA_def_cfa_offset: +3

.text
.cfi_sections .debug_frame
.globl cfi_width
.type cfi_width,@function
cfi_width:
  .cfi_startproc
  push16 r0
  .cfi_def_cfa_offset 5
  .cfi_offset 0, -5
  pop16 r0
  .cfi_def_cfa_offset 3
  .cfi_restore 0
  ret
  .cfi_endproc
.size cfi_width, .-cfi_width
