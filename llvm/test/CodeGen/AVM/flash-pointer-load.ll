; RUN: llc -mtriple=avm-unknown-arduboyfx -mcpu=avm1 -mtune=avm-interpreter-32u4-v1 \
; RUN:   < %s | FileCheck %s

; A program pointer loaded from an indexed table is a 24-bit load. LOAD24_PSEUDO
; expands it to F7LD16_POST and LD8U, a 16-bit postincrement load and an 8-bit load,
; so no 32-bit load is emitted. The flash table and the RAM table select the same
; pair. The flash table is placed in .flash with 3 bytes per entry, and a 16-bit data
; pointer from the flash table is one 16-bit load.

@prog_a = addrspace(1) constant [1 x i8] c"\01"
@prog_b = addrspace(1) constant [1 x i8] c"\02"
@fa = internal constant i8 1
@fb = internal constant i8 2
@flash_program_table = internal constant [2 x ptr addrspace(1)] [ptr addrspace(1) @prog_a, ptr addrspace(1) @prog_b]
@ram_program_table = global [2 x ptr addrspace(1)] [ptr addrspace(1) @prog_a, ptr addrspace(1) @prog_b]
@flash_data_table = internal constant [2 x ptr] [ptr @fa, ptr @fb]

define ptr addrspace(1) @indexed_flash_program_ptr(i16 %i) {
; CHECK-LABEL: indexed_flash_program_ptr:
; CHECK:       ldi16 r6, flash_program_table
; CHECK-NEXT:  add r6, r5
; CHECK-NEXT:  ld16 r4, [r6+]
; CHECK-NEXT:  ld8u r5, [r6]
; CHECK-NEXT:  ret
; CHECK-NOT:   ld32
  %p = getelementptr inbounds [2 x ptr addrspace(1)], ptr @flash_program_table, i16 0, i16 %i
  %v = load ptr addrspace(1), ptr %p, align 2
  ret ptr addrspace(1) %v
}

define ptr addrspace(1) @indexed_ram_program_ptr(i16 %i) {
; CHECK-LABEL: indexed_ram_program_ptr:
; CHECK:       ldi16 r6, ram_program_table
; CHECK-NEXT:  add r6, r5
; CHECK-NEXT:  ld16 r4, [r6+]
; CHECK-NEXT:  ld8u r5, [r6]
; CHECK-NEXT:  ret
; CHECK-NOT:   ld32
  %p = getelementptr inbounds [2 x ptr addrspace(1)], ptr @ram_program_table, i16 0, i16 %i
  %v = load ptr addrspace(1), ptr %p, align 2
  ret ptr addrspace(1) %v
}

define ptr @indexed_flash_data_ptr(i16 %i) {
; CHECK-LABEL: indexed_flash_data_ptr:
; CHECK:       ldi16 r5, flash_data_table
; CHECK-NEXT:  add r5, r4
; CHECK-NEXT:  ld16 r4, [r5]
; CHECK-NEXT:  ret
  %p = getelementptr inbounds [2 x ptr], ptr @flash_data_table, i16 0, i16 %i
  %v = load ptr, ptr %p, align 2
  ret ptr %v
}

; CHECK-LABEL: .section .flash,"a",@progbits
; CHECK:       flash_program_table:
; CHECK-NEXT:  .progptr %prog24(prog_a)
; CHECK-NEXT:  .progptr %prog24(prog_b)
; CHECK:       .size flash_program_table, 6
