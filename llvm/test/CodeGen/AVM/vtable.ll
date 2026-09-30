; RUN: llc -mtriple=avm-unknown-arduboyfx -mcpu=avm1 -O2 \
; RUN:   -verify-machineinstrs < %s | FileCheck %s
; RUN: llc -mtriple=avm-unknown-arduboyfx -mcpu=avm1 -O2 \
; RUN:   -verify-machineinstrs -filetype=obj < %s -o %t.o
; RUN: llvm-readobj --sections --relocations --symbols %t.o | \
; RUN:   FileCheck %s --check-prefix=OBJ

@rtti = global i16 0, align 1
@vtable = addrspace(1) constant [3 x ptr addrspace(1)] [
  ptr addrspace(1) null,
  ptr addrspace(1) addrspacecast (ptr @rtti to ptr addrspace(1)),
  ptr addrspace(1) @method], align 1

declare i16 @method(ptr) addrspace(1)

define i16 @invoke(ptr %object) addrspace(1) {
  %vptr = load ptr addrspace(1), ptr %object, align 1
  %slot = getelementptr ptr addrspace(1), ptr addrspace(1) %vptr, i16 0
  %callee = load ptr addrspace(1), ptr addrspace(1) %slot, align 1
  %result = call addrspace(1) i16 %callee(ptr %object)
  ret i16 %result
}

; CHECK-LABEL: invoke:
; CHECK: ldp24
; CHECK: jmpp
; CHECK-LABEL: vtable:
; CHECK: .short 0
; CHECK-NEXT: .byte 0
; CHECK: .short rtti
; CHECK-NEXT: .byte 0
; CHECK: .progptr %prog24(method)
; CHECK: .size vtable, 9
; OBJ: Name: .rodata
; OBJ: SHF_AVM_PROGSPACE
; OBJ: R_AVM_DATA16 rtti
; OBJ: R_AVM_PROG24 method
; OBJ: Name: vtable
; OBJ: Section: .rodata
