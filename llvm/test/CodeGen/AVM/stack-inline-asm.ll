; RUN: llc -mtriple=avm -O0 -verify-machineinstrs %s -o %t.s
; RUN: FileCheck %s < %t.s
; RUN: llc -mtriple=avm -O2 -verify-machineinstrs %s -o - | FileCheck %s
; RUN: llc -mtriple=avm -O0 -filetype=obj %s -o %t.direct.o
; RUN: llvm-mc -triple=avm -filetype=obj %t.s -o %t.roundtrip.o
; RUN: llvm-readobj --hex-dump=.avm.stackcalls %t.direct.o > %t.direct.txt
; RUN: llvm-readobj --hex-dump=.avm.stackcalls %t.roundtrip.o > %t.roundtrip.txt
; RUN: diff -I '^File:' %t.direct.txt %t.roundtrip.txt

; A complete function marker is 8; an opaque/dynamic function marker is 12.
; Both assembly and direct object output must use substituted MC instructions.

define void @empty() {
; CHECK-LABEL: empty:
; CHECK: .section .avm.stackcalls,
; CHECK: .short 0
; CHECK-NEXT: .byte 0
; CHECK-NEXT: .short 0
; CHECK-NEXT: .byte 8
  call void asm sideeffect "", "~{memory}"()
  ret void
}

define i16 @arithmetic(i16 %a, i16 %b) {
; CHECK-LABEL: arithmetic:
; CHECK: mul16
; CHECK: .section .avm.stackcalls,
; CHECK: .short 0
; CHECK-NEXT: .byte 0
; CHECK-NEXT: .short 0
; CHECK-NEXT: .byte 8
  %v = call i16 asm "mul16 $0, $2", "=r,0,r"(i16 %a, i16 %b)
  ret i16 %v
}

define void @local_loop() {
; CHECK-LABEL: local_loop:
; CHECK: .section .avm.stackcalls,
; CHECK: .short 0
; CHECK-NEXT: .byte 0
; CHECK-NEXT: .short 0
; CHECK-NEXT: .byte 8
  call void asm sideeffect "jmp 1f\0A1: nop\0A.Lagain: cmp r4, r5\0Abreq .Ldone\0Ajmp .Lagain\0A.Ldone:", "~{cc}"()
  ret void
}

define void @read_stack() {
; CHECK-LABEL: read_stack:
; CHECK: .section .avm.stackcalls,
; CHECK: .short 0
; CHECK-NEXT: .byte 0
; CHECK-NEXT: .short 0
; CHECK-NEXT: .byte 8
  call void asm sideeffect "getsp r4\0Aleasp r5, 0\0Aldsp16 r4, [sp+0]", "~{r4},~{r5}"()
  ret void
}

define void @fixed_frame() {
; CHECK-LABEL: fixed_frame:
; CHECK: adjsp -
; CHECK: .section .avm.stackcalls,
; CHECK: .short 0
; CHECK-NEXT: .byte 0
; CHECK-NEXT: .short 0
; CHECK-NEXT: .byte 8
; CHECK: .section .stack_sizes,
; CHECK: .byte {{[1-9][0-9]*}}
  %slot = alloca [32 x i8], align 1
  store volatile i8 1, ptr %slot
  call void asm sideeffect "nop", "~{memory}"()
  ret void
}

define void @service() {
; CHECK-LABEL: service:
; CHECK: .section .avm.stackcalls,
; CHECK: .short 0
; CHECK-NEXT: .byte 0
; CHECK-NEXT: .short 0
; CHECK-NEXT: .byte 8
  call void asm sideeffect "sys debug_break", ""()
  ret void
}

define void @stack_adjust() {
; CHECK-LABEL: stack_adjust:
; CHECK: .section .avm.stackcalls,
; CHECK: .short 0
; CHECK-NEXT: .byte 0
; CHECK-NEXT: .short 0
; CHECK-NEXT: .byte 12
  call void asm sideeffect "adjsp -2\0Aadjsp 2", ""()
  ret void
}

define void @push_pop() {
; CHECK-LABEL: push_pop:
; CHECK: .section .avm.stackcalls,
; CHECK: .short 0
; CHECK-NEXT: .byte 0
; CHECK-NEXT: .short 0
; CHECK-NEXT: .byte 12
  call void asm sideeffect "push16 r4\0Apop16 r4", ""()
  ret void
}

define void @set_stack() {
; CHECK-LABEL: set_stack:
; CHECK: .section .avm.stackcalls,
; CHECK: .short 0
; CHECK-NEXT: .byte 0
; CHECK-NEXT: .short 0
; CHECK-NEXT: .byte 12
  call void asm sideeffect "setsp r4", ""()
  ret void
}

define void @hidden_call() {
; CHECK-LABEL: hidden_call:
; CHECK: .section .avm.stackcalls,
; CHECK: .short 0
; CHECK-NEXT: .byte 0
; CHECK-NEXT: .short 0
; CHECK-NEXT: .byte 12
  call void asm sideeffect "call external", ""()
  ret void
}

define void @hidden_indirect_call() {
; CHECK-LABEL: hidden_indirect_call:
; CHECK: .section .avm.stackcalls,
; CHECK: .short 0
; CHECK-NEXT: .byte 0
; CHECK-NEXT: .short 0
; CHECK-NEXT: .byte 12
  call void asm sideeffect "callp q2", ""()
  ret void
}

define void @hidden_return() {
; CHECK-LABEL: hidden_return:
; CHECK: .section .avm.stackcalls,
; CHECK: .short 0
; CHECK-NEXT: .byte 0
; CHECK-NEXT: .short 0
; CHECK-NEXT: .byte 12
  call void asm sideeffect "ret", ""()
  ret void
}

define void @external_jump() {
; CHECK-LABEL: external_jump:
; CHECK: .section .avm.stackcalls,
; CHECK: .short 0
; CHECK-NEXT: .byte 0
; CHECK-NEXT: .short 0
; CHECK-NEXT: .byte 12
  call void asm sideeffect "jmp external", ""()
  ret void
}

define void @indirect_jump() {
; CHECK-LABEL: indirect_jump:
; CHECK: .section .avm.stackcalls,
; CHECK: .short 0
; CHECK-NEXT: .byte 0
; CHECK-NEXT: .short 0
; CHECK-NEXT: .byte 12
  call void asm sideeffect "jmpp q2", ""()
  ret void
}

define void @offset_jump() {
; CHECK-LABEL: offset_jump:
; CHECK: .section .avm.stackcalls,
; CHECK: .short 0
; CHECK-NEXT: .byte 0
; CHECK-NEXT: .short 0
; CHECK-NEXT: .byte 12
  call void asm sideeffect ".Loffset: nop\0Ajmp .Loffset+100", ""()
  ret void
}

define void @raw_instruction() {
; CHECK-LABEL: raw_instruction:
; CHECK: .section .avm.stackcalls,
; CHECK: .short 0
; CHECK-NEXT: .byte 0
; CHECK-NEXT: .short 0
; CHECK-NEXT: .byte 12
  call void asm sideeffect ".byte 0xb4", ""()
  ret void
}

define void @label_then_directive() {
; CHECK-LABEL: label_then_directive:
; CHECK: .section .avm.stackcalls,
; CHECK: .short 0
; CHECK-NEXT: .byte 0
; CHECK-NEXT: .short 0
; CHECK-NEXT: .byte 12
  call void asm sideeffect ".Lbytes: .byte 0xb4", ""()
  ret void
}

define void @macro_instruction() {
; CHECK-LABEL: macro_instruction:
; CHECK: .section .avm.stackcalls,
; CHECK: .short 0
; CHECK-NEXT: .byte 0
; CHECK-NEXT: .short 0
; CHECK-NEXT: .byte 12
  call void asm sideeffect ".macro hidden\0Apush16 r4\0A.endm\0Ahidden", ""()
  ret void
}

define void @mixed_blocks() {
; CHECK-LABEL: mixed_blocks:
; CHECK: .section .avm.stackcalls,
; CHECK: .short 0
; CHECK-NEXT: .byte 0
; CHECK-NEXT: .short 0
; CHECK-NEXT: .byte 12
  call void asm sideeffect "adjsp -2", ""()
  call void asm sideeffect "nop", ""()
  ret void
}

define void @dynamic(i16 %n) {
; CHECK-LABEL: dynamic:
; CHECK: .section .avm.stackcalls,
; CHECK: .short 0
; CHECK-NEXT: .byte 0
; CHECK-NEXT: .short 0
; CHECK-NEXT: .byte 12
  %slot = alloca i8, i16 %n, align 1
  store volatile i8 1, ptr %slot
  call void asm sideeffect "", "~{memory}"()
  ret void
}

define void @quoted_directive() {
; CHECK-LABEL: quoted_directive:
; CHECK: .section .avm.stackcalls,
; CHECK: .short 0
; CHECK-NEXT: .byte 0
; CHECK-NEXT: .short 0
; CHECK-NEXT: .byte 12
  call void asm sideeffect "\22.byte\22 0xb4", ""()
  ret void
}

define void @unknown_service() {
; CHECK-LABEL: unknown_service:
; CHECK: .section .avm.stackcalls,
; CHECK: .short 0
; CHECK-NEXT: .byte 0
; CHECK-NEXT: .short 0
; CHECK-NEXT: .byte 12
  call void asm sideeffect "sys 255", ""()
  ret void
}

define void @literal_jump() {
; CHECK-LABEL: literal_jump:
; CHECK: .section .avm.stackcalls,
; CHECK: .short 0
; CHECK-NEXT: .byte 0
; CHECK-NEXT: .short 0
; CHECK-NEXT: .byte 12
  call void asm sideeffect "jmp8 -2", ""()
  ret void
}

define void @macro_provider() {
; CHECK-LABEL: macro_provider:
; CHECK: .section .avm.stackcalls,
; CHECK: .short 0
; CHECK-NEXT: .byte 0
; CHECK-NEXT: .short 0
; CHECK-NEXT: .byte 12
  call void asm sideeffect ".macro nop\0Apush16 r4\0A.endm", ""()
  ret void
}

define void @macro_shadow() {
; CHECK-LABEL: macro_shadow:
; CHECK: push16 r4
; CHECK: .section .avm.stackcalls,
; CHECK: .short 0
; CHECK-NEXT: .byte 0
; CHECK-NEXT: .short 0
; CHECK-NEXT: .byte 12
  call void asm sideeffect "nop", ""()
  ret void
}
