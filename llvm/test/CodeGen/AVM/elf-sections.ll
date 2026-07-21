; RUN: llc -mtriple=avm-unknown-arduboyfx -filetype=obj %s -o %t.default.o
; RUN: llvm-readobj --sections %t.default.o | FileCheck %s --check-prefix=DEFAULT
; RUN: llc -mtriple=avm-unknown-arduboyfx \
; RUN:   -function-sections -data-sections \
; RUN:   -filetype=obj %s -o %t.split.o
; RUN: llvm-readobj --sections %t.split.o | FileCheck %s --check-prefix=SPLIT

target triple = "avm-unknown-arduboyfx"

$grouped = comdat any

@ram_zero = addrspace(0) global i16 0, align 1
@ram_const = addrspace(0) constant i16 7, align 1
@prog_const = addrspace(1) constant i16 9, align 1

define void @first() {
  ret void
}

define void @second() {
  ret void
}

define linkonce_odr void @grouped() comdat {
  ret void
}

; DEFAULT:      Name: .text
; DEFAULT:      Type: SHT_PROGBITS
; DEFAULT:      Flags [
; DEFAULT-NEXT:   SHF_ALLOC
; DEFAULT-NEXT:   SHF_AVM_PROGSPACE
; DEFAULT-NEXT:   SHF_EXECINSTR
; DEFAULT-NEXT: ]
; DEFAULT:      AddressAlignment: 1
; DEFAULT:      Name: .data
; DEFAULT:      Type: SHT_PROGBITS
; DEFAULT:      Flags [
; DEFAULT-NEXT:   SHF_ALLOC
; DEFAULT-NEXT:   SHF_AVM_DATASPACE
; DEFAULT-NEXT:   SHF_WRITE
; DEFAULT-NEXT: ]
; DEFAULT:      Name: .rodata
; DEFAULT:      Type: SHT_PROGBITS
; DEFAULT:      Flags [
; DEFAULT-NEXT:   SHF_ALLOC
; DEFAULT-NEXT:   SHF_AVM_PROGSPACE
; DEFAULT-NEXT: ]

; SPLIT:      Name: .text.first
; SPLIT:      Type: SHT_PROGBITS
; SPLIT:      Flags [
; SPLIT-NEXT:   SHF_ALLOC
; SPLIT-NEXT:   SHF_AVM_PROGSPACE
; SPLIT-NEXT:   SHF_EXECINSTR
; SPLIT-NEXT: ]
; SPLIT:      AddressAlignment: 1
; SPLIT:      Name: .text.second
; SPLIT:      Type: SHT_PROGBITS
; SPLIT:      Flags [
; SPLIT-NEXT:   SHF_ALLOC
; SPLIT-NEXT:   SHF_AVM_PROGSPACE
; SPLIT-NEXT:   SHF_EXECINSTR
; SPLIT-NEXT: ]
; SPLIT:      AddressAlignment: 1
; SPLIT:      Name: .text.grouped
; SPLIT:      Type: SHT_PROGBITS
; SPLIT:      Flags [
; SPLIT-NEXT:   SHF_ALLOC
; SPLIT-NEXT:   SHF_AVM_PROGSPACE
; SPLIT-NEXT:   SHF_EXECINSTR
; SPLIT-NEXT:   SHF_GROUP
; SPLIT-NEXT: ]
; SPLIT:      AddressAlignment: 1
; SPLIT:      Name: .data.ram_zero
; SPLIT:      Type: SHT_PROGBITS
; SPLIT:      Flags [
; SPLIT-NEXT:   SHF_ALLOC
; SPLIT-NEXT:   SHF_AVM_DATASPACE
; SPLIT-NEXT:   SHF_WRITE
; SPLIT-NEXT: ]
; SPLIT:      Name: .data.ram_const
; SPLIT:      Type: SHT_PROGBITS
; SPLIT:      Flags [
; SPLIT-NEXT:   SHF_ALLOC
; SPLIT-NEXT:   SHF_AVM_DATASPACE
; SPLIT-NEXT:   SHF_WRITE
; SPLIT-NEXT: ]
; SPLIT:      Name: .rodata.prog_const
; SPLIT:      Type: SHT_PROGBITS
; SPLIT:      Flags [
; SPLIT-NEXT:   SHF_ALLOC
; SPLIT-NEXT:   SHF_AVM_PROGSPACE
; SPLIT-NEXT: ]
