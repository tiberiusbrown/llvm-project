; RUN: llc -mtriple=avm-unknown-arduboyfx -filetype=obj < %s -o %t.o
; RUN: llvm-readobj --sections --relocations %t.o | FileCheck %s

@ordinary = global i16 1, align 1
@zero = global [3 x i8] zeroinitializer, align 1
@constant = constant [4 x i8] c"avm\00", align 1
@saved = global i8 7, section ".saved.config", align 1
@program = addrspace(1) constant [2 x i8] c"p\00", align 1

define i16 @read_ordinary() {
  %value = load i16, ptr @ordinary, align 1
  ret i16 %value
}

; CHECK: Name: .text
; CHECK: SHF_AVM_PROGSPACE
; CHECK: Name: .data
; CHECK: SHF_AVM_DATASPACE
; CHECK: Name: .saved.config
; CHECK: SHF_AVM_DATASPACE
; CHECK: Name: .rodata
; CHECK: SHF_AVM_PROGSPACE
; CHECK: R_AVM_DATA16 ordinary 0x0
