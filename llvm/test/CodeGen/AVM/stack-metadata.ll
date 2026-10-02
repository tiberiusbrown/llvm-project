; RUN: llc -mtriple=avm -O0 -verify-machineinstrs %s -o %t.s
; RUN: FileCheck %s --check-prefix=ASM < %t.s
; RUN: llvm-mc -triple=avm -filetype=obj %t.s -o %t.o
; RUN: llvm-readobj --stack-sizes --relocations %t.o | FileCheck %s --check-prefix=OBJ
; RUN: llc -mtriple=avm -O0 -verify-machineinstrs -filetype=obj %s -o %t.direct.o
; RUN: llvm-readobj --stack-sizes --relocations %t.direct.o | FileCheck %s --check-prefix=OBJ

; Automatic metadata, without -stack-size-section. Frame includes a 40-byte
; local and late callee-save/spill objects. Two calls have distinct argument
; areas. A tail-position IR call remains an ordinary machine call.
; ASM-LABEL: caller:
; ASM: .section .avm.stackcalls,"o",
; ASM: .3byte {{.*}}
; ASM: .3byte leaf
; ASM: .short 0
; ASM: .byte 0
; ASM: .short 6
; ASM: .byte 0
; ASM: .section .stack_sizes,"o",
; ASM: .byte 42
; ASM-LABEL: dynamic:
; ASM: .section .stack_sizes,"o",
; ASM: .byte {{[0-9]+}}
; ASM: .section .avm.stackcalls,"o",
; ASM: .byte 12
; ASM-LABEL: indirect:
; ASM: .short 0
; ASM: .byte 3
; ASM: .byte 8
; ASM-LABEL: framed_indirect:
; ASM: adjsp 1
; ASM-NEXT: jmpp q2
; ASM-NOT: ret
; ASM: .short 0
; ASM: .byte 3
; ASM-LABEL: tail_forward:
; ASM: .3byte leaf
; ASM: .short 0
; ASM: .byte 2
; ASM-LABEL: framed_direct:
; ASM: adjsp 8
; ASM-NEXT: jmp leaf
; ASM-NOT: ret
; ASM: .section .avm.stackcalls,"o",
; ASM: .3byte leaf
; ASM: .short 0
; ASM: .byte 2
; OBJ: .rela.avm.stackcalls
; OBJ: R_AVM_DEBUG24
; OBJ: leaf
; OBJ: .rela.stack_sizes
; OBJ: StackSizes [
; OBJ: Size: 0x2A

declare void @leaf()
declare void @args(i16, i16, i16, i16, i16, i16, i16)

define void @caller() {
  %a = alloca [40 x i8], align 1
  %p = getelementptr [40 x i8], ptr %a, i16 0, i16 0
  store volatile i8 1, ptr %p
  call void @leaf()
  tail call void @args(i16 1, i16 2, i16 3, i16 4, i16 5, i16 6, i16 7)
  ret void
}

define void @dynamic(i16 %size) {
  %a = alloca i8, i16 %size, align 1
  store volatile i8 1, ptr %a
  call void @leaf()
  ret void
}

define void @indirect(ptr addrspace(1) %f) {
  call addrspace(1) void %f()
  ret void
}

define void @framed_indirect(ptr addrspace(1) %f) {
  %a = alloca i8, align 1
  store volatile i8 1, ptr %a
  tail call addrspace(1) void %f()
  ret void
}

define void @tail_forward() {
  call void @leaf()
  ret void
}

define void @framed_direct() {
  %a = alloca [8 x i8], align 1
  store volatile i8 1, ptr %a
  tail call void @leaf()
  ret void
}
