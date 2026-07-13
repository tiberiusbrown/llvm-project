; RUN: llc -mtriple=avm-unknown-arduboyfx -filetype=obj < %s -o %t.o
; RUN: llvm-readobj -r %t.o | FileCheck %s --check-prefix=RELOC
; RUN: llc -mtriple=avm-unknown-arduboyfx < %s | FileCheck %s --check-prefix=ASM

@defined = global i16 3, align 1
@bytes = global [4 x i8] c"ABCD", align 1
@external = external global i16, align 1

define ptr @defined_address() {
  ret ptr @defined
}

define ptr @defined_addend() {
  ret ptr getelementptr ([4 x i8], ptr @bytes, i16 0, i16 2)
}

define ptr @external_address() {
  ret ptr @external
}

; ASM: ldi16 {{.*}}, defined
; ASM: ldi16 {{.*}}, bytes+2
; ASM: ldi16 {{.*}}, external
; RELOC: R_AVM_DATA16 defined
; RELOC: R_AVM_DATA16 bytes 0x2
; RELOC: R_AVM_DATA16 external
