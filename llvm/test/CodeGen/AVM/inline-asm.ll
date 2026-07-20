; RUN: llc -mtriple=avm-unknown-arduboyfx -mcpu=avm1 \
; RUN:   -mtune=avm-interpreter-32u4-v1 -O0 -verify-machineinstrs < %s \
; RUN:   | FileCheck %s
; RUN: llc -mtriple=avm-unknown-arduboyfx -mcpu=avm1 \
; RUN:   -mtune=avm-interpreter-32u4-v1 -O2 -verify-machineinstrs < %s \
; RUN:   | FileCheck %s

define void @register_constraints(i16 %word, i8 %byte, i32 %pair,
                                  ptr addrspace(1) %program) {
; CHECK-LABEL: register_constraints:
; CHECK:       ; r{{[0-7]}}
; CHECK:       ; r{{[4-7]}}
; CHECK:       ; r{{[0-7]}}
; CHECK:       ; r{{[4-7]}}
; CHECK:       ; r{{[0-7]}}
; CHECK:       ; r{{[4-7]}}
; CHECK:       ; q{{[0-3]}}
; CHECK:       ; q{{[2-3]}}
; CHECK:       ; q{{[0-3]}}
  call void asm sideeffect "nop ; $0", "r"(i16 %word)
  call void asm sideeffect "nop ; $0", "c"(i16 %word)
  call void asm sideeffect "nop ; $0", "b"(i8 %byte)
  call void asm sideeffect "nop ; $0", "B"(i8 %byte)
  call void asm sideeffect "nop ; $0", "p"(i16 %word)
  call void asm sideeffect "nop ; $0", "P"(i16 %word)
  call void asm sideeffect "nop ; $0", "q"(i32 %pair)
  call void asm sideeffect "nop ; $0", "Q"(i32 %pair)
  call void asm sideeffect "nop ; $0", "t"(ptr addrspace(1) %program)
  ret void
}

define void @fixed_registers(i16 %word, i32 %pair) {
; CHECK-LABEL: fixed_registers:
; CHECK:       ; r0
; CHECK:       ; r7
; CHECK:       ; q0
; CHECK:       ; q3
  call void asm sideeffect "nop ; $0", "{r0}"(i16 %word)
  call void asm sideeffect "nop ; $0", "{r7}"(i16 %word)
  call void asm sideeffect "nop ; $0", "{q0}"(i32 %pair)
  call void asm sideeffect "nop ; $0", "{q3}"(i32 %pair)
  ret void
}

define void @computed_program_pointer_constraints(ptr addrspace(1) %base,
                                                   i32 %offset) {
; CHECK-LABEL: computed_program_pointer_constraints:
; CHECK:       add32
; CHECK-NOT:   and
; CHECK-NOT:   zext8
; CHECK:       ; q{{[0-3]}}
; CHECK:       add32
; CHECK:       and
; CHECK:       ; q{{[0-3]}}
  %semantic = getelementptr i8, ptr addrspace(1) %base, i32 %offset
  call void asm sideeffect "nop ; $0", "t"(ptr addrspace(1) %semantic)
  %observed = getelementptr i8, ptr addrspace(1) %semantic, i32 %offset
  call void asm sideeffect "nop ; $0", "q"(ptr addrspace(1) %observed)
  ret void
}

define void @immediate_constraints() {
; CHECK-LABEL: immediate_constraints:
; CHECK:       ; -128
; CHECK:       ; 255
; CHECK:       ; 15
; CHECK:       ; -32768
; CHECK:       ; 65535
; CHECK:       ; 15
; CHECK:       ; 0
  call void asm sideeffect "nop ; $0", "I"(i16 -128)
  call void asm sideeffect "nop ; $0", "J"(i16 255)
  call void asm sideeffect "nop ; $0", "K"(i16 15)
  call void asm sideeffect "nop ; $0", "L"(i16 -32768)
  call void asm sideeffect "nop ; $0", "M"(i16 -1)
  call void asm sideeffect "nop ; $0", "N"(i16 15)
  call void asm sideeffect "nop ; $0", "O"(i16 0)
  ret void
}

define void @memory_constraints(ptr %address) {
; CHECK-LABEL: memory_constraints:
; CHECK:       ; {{.*r[0-7].*}}
; CHECK:       ; {{.*r[0-7].*}}
  call void asm sideeffect "nop ; $0", "*m"(ptr elementtype(i16) %address)
  call void asm sideeffect "nop ; $0", "*o"(ptr elementtype(i16) %address)
  ret void
}

define i16 @modifiers(i16 %value) {
; CHECK-LABEL: modifiers:
; CHECK:       ; [[REG:r[0-7]]], [[REG]]
  %result = call i16 asm sideeffect "nop ; $0, $1", "=&r,0"(i16 %value)
  ret i16 %result
}

define void @clobbers() {
; CHECK-LABEL: clobbers:
; CHECK:       nop
  call void asm sideeffect "nop", "~{cc},~{memory}"()
  ret void
}
