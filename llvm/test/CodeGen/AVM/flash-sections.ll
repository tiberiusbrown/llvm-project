; RUN: llc -mtriple=avm-unknown-arduboyfx -filetype=obj %s -o %t.default.o
; RUN: llvm-readobj --sections --relocations %t.default.o | FileCheck %s --check-prefix=DEFAULT
; RUN: llc -mtriple=avm-unknown-arduboyfx -function-sections -data-sections \
; RUN:   -filetype=obj %s -o %t.split.o
; RUN: llvm-readobj --sections --relocations %t.split.o | FileCheck %s --check-prefix=SPLIT
; RUN: llc -mtriple=avm-unknown-arduboyfx -filetype=asm %s -o - | FileCheck %s --check-prefix=ASM

target triple = "avm-unknown-arduboyfx"

$grouped = comdat any

@table = internal constant [4 x i8] c"\01\02\03\04", align 1
@zeroed = internal constant [3 x i8] zeroinitializer, align 1
@message = private unnamed_addr constant [6 x i8] c"Hello\00", align 1
@pointers = internal constant [2 x ptr] [ptr @table, ptr @message], align 1
@grouped = linkonce_odr constant [2 x i8] c"gr", comdat, align 1
@writable = internal global [2 x i8] c"ab", align 1
@writable_pointer = internal global ptr @message, align 1
@prog_table = addrspace(1) constant [2 x i8] c"pq", align 1

define i8 @read_table(i8 %index) {
  %slot = getelementptr inbounds i8, ptr @table, i8 %index
  %value = load i8, ptr %slot, align 1
  ret i8 %value
}

define ptr @get_message() {
  %value = load ptr, ptr @writable_pointer, align 1
  ret ptr %value
}

define ptr @get_pointers() {
  ret ptr @pointers
}

define i8 @read_grouped() {
  %value = load i8, ptr @grouped, align 1
  ret i8 %value
}

define i8 @read_zeroed() {
  %value = load i8, ptr @zeroed, align 1
  ret i8 %value
}

define i8 @read_writable() {
  %value = load i8, ptr @writable, align 1
  ret i8 %value
}

define i8 @read_program() {
  %value = load i8, ptr addrspace(1) @prog_table, align 1
  ret i8 %value
}

; DEFAULT:      Name: .flash
; DEFAULT-NEXT: Type: SHT_PROGBITS
; DEFAULT-NEXT: Flags [
; DEFAULT-NEXT:   SHF_ALLOC
; DEFAULT-NEXT:   SHF_AVM_DATASPACE
; DEFAULT-NEXT: ]
; DEFAULT:      Name: .flash.str1.1
; DEFAULT-NEXT: Type: SHT_PROGBITS
; DEFAULT-NEXT: Flags [
; DEFAULT-NEXT:   SHF_ALLOC
; DEFAULT-NEXT:   SHF_AVM_DATASPACE
; DEFAULT-NEXT:   SHF_MERGE
; DEFAULT-NEXT:   SHF_STRINGS
; DEFAULT-NEXT: ]
; DEFAULT:      Name: .flash.grouped
; DEFAULT-NEXT: Type: SHT_PROGBITS
; DEFAULT-NEXT: Flags [
; DEFAULT-NEXT:   SHF_ALLOC
; DEFAULT-NEXT:   SHF_AVM_DATASPACE
; DEFAULT-NEXT:   SHF_GROUP
; DEFAULT-NEXT: ]
; DEFAULT:      Name: .data
; DEFAULT-NEXT: Type: SHT_PROGBITS
; DEFAULT-NEXT: Flags [
; DEFAULT-NEXT:   SHF_ALLOC
; DEFAULT-NEXT:   SHF_AVM_DATASPACE
; DEFAULT-NEXT:   SHF_WRITE
; DEFAULT-NEXT: ]
; DEFAULT:      Name: .rodata
; DEFAULT-NEXT: Type: SHT_PROGBITS
; DEFAULT-NEXT: Flags [
; DEFAULT-NEXT:   SHF_ALLOC
; DEFAULT-NEXT:   SHF_AVM_PROGSPACE
; DEFAULT-NEXT: ]
; DEFAULT-NOT:  Name: .rodata.
; DEFAULT:      R_AVM_DATA16 .flash 0x0
; DEFAULT:      R_AVM_DATA16 .flash.str1.1 0x0

; SPLIT:      Name: .flash.table
; SPLIT-NEXT: Type: SHT_PROGBITS
; SPLIT-NEXT: Flags [
; SPLIT-NEXT:   SHF_ALLOC
; SPLIT-NEXT:   SHF_AVM_DATASPACE
; SPLIT-NEXT: ]
; SPLIT:      Name: .flash.zeroed
; SPLIT-NEXT: Type: SHT_PROGBITS
; SPLIT-NEXT: Flags [
; SPLIT-NEXT:   SHF_ALLOC
; SPLIT-NEXT:   SHF_AVM_DATASPACE
; SPLIT-NEXT: ]
; SPLIT:      Name: .flash.str1.1
; SPLIT-NEXT: Type: SHT_PROGBITS
; SPLIT-NEXT: Flags [
; SPLIT-NEXT:   SHF_ALLOC
; SPLIT-NEXT:   SHF_AVM_DATASPACE
; SPLIT-NEXT:   SHF_MERGE
; SPLIT-NEXT:   SHF_STRINGS
; SPLIT-NEXT: ]
; SPLIT:      Name: .flash.pointers
; SPLIT-NEXT: Type: SHT_PROGBITS
; SPLIT-NEXT: Flags [
; SPLIT-NEXT:   SHF_ALLOC
; SPLIT-NEXT:   SHF_AVM_DATASPACE
; SPLIT-NEXT: ]
; SPLIT:      Name: .flash.grouped
; SPLIT-NEXT: Type: SHT_PROGBITS
; SPLIT-NEXT: Flags [
; SPLIT-NEXT:   SHF_ALLOC
; SPLIT-NEXT:   SHF_AVM_DATASPACE
; SPLIT-NEXT:   SHF_GROUP
; SPLIT-NEXT: ]
; SPLIT:      Name: .data.writable
; SPLIT-NEXT: Type: SHT_PROGBITS
; SPLIT-NEXT: Flags [
; SPLIT-NEXT:   SHF_ALLOC
; SPLIT-NEXT:   SHF_AVM_DATASPACE
; SPLIT-NEXT:   SHF_WRITE
; SPLIT-NEXT: ]
; SPLIT:      Name: .rodata.prog_table
; SPLIT-NEXT: Type: SHT_PROGBITS
; SPLIT-NEXT: Flags [
; SPLIT-NEXT:   SHF_ALLOC
; SPLIT-NEXT:   SHF_AVM_PROGSPACE
; SPLIT-NEXT: ]
; SPLIT:      R_AVM_DATA16 .flash.table 0x0
; SPLIT:      R_AVM_DATA16 .flash.str1.1 0x0

; ASM: .section .flash,"a",@progbits
; ASM: .section .flash.str1.1,"aMS",@progbits,1
; ASM: .section .flash,"a",@progbits
; ASM: .section .flash.grouped,"aG",@progbits,grouped,comdat
; ASM: .section .rodata,"a",@progbits
