; RUN: llc -mtriple=avm-unknown-arduboyfx -O2 -verify-machineinstrs -filetype=obj %s -o %t.o
; RUN: llvm-readobj --sections %t.o | FileCheck %s --check-prefix=SECTIONS
; RUN: llvm-dwarfdump --debug-frame %t.o | FileCheck %s --check-prefix=DWARF

; An unwind table is requested without debug information. Its CFI must land in
; the nonallocated .debug_frame, never in the allocated .eh_frame.
; SECTIONS: Name: .debug_frame
; SECTIONS-NEXT: Type: SHT_PROGBITS
; SECTIONS-NEXT: Flags [ (0x0)
; SECTIONS-NOT: Name: .eh_frame

; DWARF: CIE
; DWARF: FDE cie=00000000 pc=00000000...00000003
; DWARF-NOT: FDE

define i16 @unwound(i16 %x) uwtable {
  %y = add i16 %x, 1
  ret i16 %y
}

; A function without an unwind table and without debug information has no FDE.
define i16 @plain(i16 %x) nounwind {
  %y = add i16 %x, 2
  ret i16 %y
}
