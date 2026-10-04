; RUN: split-file %s %t
; RUN: llc -mtriple=avm -O0 %t/memory.ll -o - | FileCheck %s --check-prefix=MEMORY
; RUN: llc -mtriple=avm -O2 %t/memory.ll -o - | FileCheck %s --check-prefix=MEMORY
; RUN: llc -mtriple=avm -O0 %t/asm.ll -o - | FileCheck %s --check-prefix=ASM

; Partial writes and dynamic offsets must not retain the initial callback as
; a complete target set. Unknown values must still propagate alias provenance.
; MEMORY-LABEL: partial_write:
; MEMORY: .short 1
; MEMORY-NEXT: .byte 0
; MEMORY-NEXT: .short 0
; MEMORY-NEXT: .byte {{[13]}}
; MEMORY-LABEL: variable_write:
; MEMORY: .short 1
; MEMORY-NEXT: .byte 0
; MEMORY-NEXT: .short 0
; MEMORY-NEXT: .byte {{[13]}}
; MEMORY-LABEL: volatile_alias:
; MEMORY: .short 1
; MEMORY-NEXT: .byte 0
; MEMORY-NEXT: .short 0
; MEMORY-NEXT: .byte {{[13]}}
; ASM-LABEL: opaque_writer:
; ASM: .short 1
; ASM-NEXT: .byte 0
; ASM-NEXT: .short 0
; ASM-NEXT: .byte {{[13]}}
; Stack certification does not weaken the memory clobber's pointer-flow gap.
; ASM: .short 0
; ASM-NEXT: .byte 0
; ASM-NEXT: .short 0
; ASM-NEXT: .byte 8

;--- memory.ll
target datalayout = "e-P1-p:16:8-p1:24:8-i8:8-i16:8-i24:8-i32:8-i64:8-f32:8-f64:8-a:8-n8:16-S8"
declare void @small() addrspace(1)
declare void @large() addrspace(1)
declare void @writer(ptr) addrspace(1)

define void @partial_write() addrspace(1) {
  %slot = alloca ptr addrspace(1), align 1
  store ptr addrspace(1) @small, ptr %slot, align 1
  store i8 1, ptr %slot, align 1
  %f = load ptr addrspace(1), ptr %slot, align 1
  call addrspace(1) void %f()
  ret void
}
define void @variable_write(i16 %offset) addrspace(1) {
  %slot = alloca [2 x ptr addrspace(1)], align 1
  store ptr addrspace(1) @small, ptr %slot, align 1
  %where = getelementptr ptr addrspace(1), ptr %slot, i16 %offset
  store ptr addrspace(1) @large, ptr %where, align 1
  %f = load ptr addrspace(1), ptr %slot, align 1
  call addrspace(1) void %f()
  ret void
}
define void @volatile_alias() addrspace(1) {
  %slot = alloca ptr addrspace(1), align 1
  %alias = alloca ptr, align 1
  store ptr addrspace(1) @small, ptr %slot, align 1
  store ptr %slot, ptr %alias, align 1
  %loaded_alias = load volatile ptr, ptr %alias, align 1
  call addrspace(1) void @writer(ptr %loaded_alias)
  %f = load ptr addrspace(1), ptr %slot, align 1
  call addrspace(1) void %f()
  ret void
}

;--- asm.ll
target datalayout = "e-P1-p:16:8-p1:24:8-i8:8-i16:8-i24:8-i32:8-i64:8-f32:8-f64:8-a:8-n8:16-S8"
@object = internal global { i8, ptr addrspace(1) } { i8 0, ptr addrspace(1) @small }, align 1
declare void @small() addrspace(1)
define void @opaque_writer() addrspace(1) {
  call void asm sideeffect "nop", "~{memory}"()
  %field = getelementptr { i8, ptr addrspace(1) }, ptr @object, i16 0, i32 1
  %f = load ptr addrspace(1), ptr %field, align 1
  call addrspace(1) void %f()
  ret void
}
