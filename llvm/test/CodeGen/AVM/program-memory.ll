; RUN: llc -mtriple=avm-unknown-arduboyfx -mcpu=avm1 \
; RUN:   -mtune=avm-interpreter-32u4-v1 -O2 -verify-machineinstrs < %s \
; RUN:   | FileCheck %s
; RUN: llc -mtriple=avm-unknown-arduboyfx -mcpu=avm1 \
; RUN:   -mtune=avm-interpreter-32u4-v1 -O2 -verify-machineinstrs \
; RUN:   -filetype=obj < %s -o %t.o
; RUN: llvm-readobj --sections --relocations --symbols %t.o \
; RUN:   | FileCheck %s --check-prefix=OBJ

; OBJ: Name: .rodata
; OBJ: Size: 27
; OBJ: Name: .rela.rodata
; OBJ: Name: .data
; OBJ: Size: 12
; OBJ: Name: .rela.data
; OBJ: Section {{.*}} .rela.rodata {
; OBJ: R_AVM_PROG24 callback
; OBJ: R_AVM_PROG24 callback
; OBJ: Section {{.*}} .rela.data {
; OBJ: R_AVM_PROG24 callback
; OBJ: R_AVM_PROG24 callback
; OBJ: R_AVM_PROG24 callback
; OBJ: Name: flash_callback
; OBJ: Size: 3
; OBJ: Section: .rodata
; OBJ: Name: data_callback
; OBJ: Size: 3
; OBJ: Section: .data
; OBJ: Name: data_callback_array
; OBJ: Size: 9
; OBJ: Section: .data
; OBJ: Name: flash_callback_array
; OBJ: Size: 6
; OBJ: Section: .rodata

@flash_bytes = addrspace(1) constant [8 x i8] c"AVM\00test", align 1
@flash_word = addrspace(1) constant i16 4660, align 1
@flash_int = addrspace(1) constant i32 305419896, align 1
@flash_float = addrspace(1) constant float 1.500000e+00, align 1
@flash_callback = addrspace(1) constant ptr addrspace(1) @callback, align 1
@data_callback = global ptr addrspace(1) @callback, align 1
@data_callback_array = global [3 x ptr addrspace(1)]
    [ptr addrspace(1) @callback, ptr addrspace(1) null,
     ptr addrspace(1) @callback], align 1
@flash_callback_array = addrspace(1) constant [2 x ptr addrspace(1)]
    [ptr addrspace(1) @callback, ptr addrspace(1) null], align 1

define i16 @callback(i16 %value) addrspace(1) {
  ret i16 %value
}

define zeroext i8 @load_program_byte() {
; CHECK-LABEL: load_program_byte:
; CHECK:       ldi16
; CHECK:       ldi8
; CHECK:       ldp8u
  %value = load i8, ptr addrspace(1) @flash_bytes, align 1
  ret i8 %value
}

define i16 @load_program_signed_byte(ptr addrspace(1) %address) {
; CHECK-LABEL: load_program_signed_byte:
; CHECK:       ldp8s
  %value = load i8, ptr addrspace(1) %address, align 1
  %extended = sext i8 %value to i16
  ret i16 %extended
}

define i16 @load_program_word() {
; CHECK-LABEL: load_program_word:
; CHECK:       ldp16
  %value = load i16, ptr addrspace(1) @flash_word, align 1
  ret i16 %value
}

define i32 @load_program_i32() {
; CHECK-LABEL: load_program_i32:
; CHECK:       ldp32
  %value = load i32, ptr addrspace(1) @flash_int, align 1
  ret i32 %value
}

define float @load_program_f32() {
; CHECK-LABEL: load_program_f32:
; CHECK:       ldp32
  %value = load float, ptr addrspace(1) @flash_float, align 1
  ret float %value
}

define i16 @load_and_call_program_pointer(i16 %value) {
; CHECK-LABEL: load_and_call_program_pointer:
; CHECK:       ldp24
; CHECK:       callp
  %callee = load ptr addrspace(1), ptr addrspace(1) @flash_callback, align 1
  %result = call addrspace(1) i16 %callee(i16 %value)
  ret i16 %result
}

define i16 @load_and_call_data_pointer(i16 %value) {
; CHECK-LABEL: load_and_call_data_pointer:
; CHECK:       ld16
; CHECK:       ld8u
; CHECK:       callp
  %callee = load ptr addrspace(1), ptr @data_callback, align 1
  %result = call addrspace(1) i16 %callee(i16 %value)
  ret i16 %result
}

define zeroext i8 @program_pointer_add(ptr addrspace(1) %base, i32 %offset) {
; CHECK-LABEL: program_pointer_add:
; CHECK:       add32
; CHECK-NOT:   zext8
; CHECK:       ldp8u
  %address = getelementptr i8, ptr addrspace(1) %base, i32 %offset
  %value = load i8, ptr addrspace(1) %address, align 1
  ret i8 %value
}

define i1 @program_pointer_is_null(ptr addrspace(1) %pointer) {
; CHECK-LABEL: program_pointer_is_null:
; CHECK:       zext8
; CHECK:       cmp32
  %result = icmp eq ptr addrspace(1) %pointer, null
  ret i1 %result
}

define i1 @program_pointer_equal(ptr addrspace(1) %left,
                                 ptr addrspace(1) %right) {
; CHECK-LABEL: program_pointer_equal:
; CHECK:       zext8
; CHECK:       cmp32
  %result = icmp eq ptr addrspace(1) %left, %right
  ret i1 %result
}

define zeroext i8 @inttoptr_program_load(i32 %bits) {
; CHECK-LABEL: inttoptr_program_load:
; CHECK-NOT:   and
; CHECK-NOT:   zext8
; CHECK:       ldp8u
  %pointer = inttoptr i32 %bits to ptr addrspace(1)
  %value = load i8, ptr addrspace(1) %pointer, align 1
  ret i8 %value
}

define i32 @program_ptrtoint(ptr addrspace(1) %pointer) {
; CHECK-LABEL: program_ptrtoint:
; CHECK:       and
; CHECK:       ret
  %bits = ptrtoint ptr addrspace(1) %pointer to i32
  ret i32 %bits
}

declare void @consume_program_pointer(ptr addrspace(1))

define void @computed_program_argument(ptr addrspace(1) %base, i32 %offset) {
; CHECK-LABEL: computed_program_argument:
; CHECK:       add32
; CHECK:       zext8
; CHECK:       call consume_program_pointer
  %pointer = getelementptr i8, ptr addrspace(1) %base, i32 %offset
  call void @consume_program_pointer(ptr addrspace(1) %pointer)
  ret void
}

define ptr addrspace(1) @computed_program_return(ptr addrspace(1) %base,
                                                 i32 %offset) {
; CHECK-LABEL: computed_program_return:
; CHECK:       add32
; CHECK:       zext8
; CHECK:       ret
  %pointer = getelementptr i8, ptr addrspace(1) %base, i32 %offset
  ret ptr addrspace(1) %pointer
}

define void @computed_indirect_call(ptr addrspace(1) %callee, i32 %offset) {
; CHECK-LABEL: computed_indirect_call:
; CHECK:       add32
; CHECK-NOT:   zext8
; CHECK:       callp
  %adjusted = getelementptr i8, ptr addrspace(1) %callee, i32 %offset
  call addrspace(1) void %adjusted()
  ret void
}

define void @computed_packed_store(ptr %slot, ptr addrspace(1) %base,
                                   i32 %offset) {
; CHECK-LABEL: computed_packed_store:
; CHECK:       add32
; CHECK-NOT:   zext8
; CHECK:       st16
; CHECK:       st8
  %pointer = getelementptr i8, ptr addrspace(1) %base, i32 %offset
  store ptr addrspace(1) %pointer, ptr %slot, align 1
  ret void
}

define zeroext i8 @postincrement_program_byte(ptr addrspace(1) %address,
                                               ptr %updated) {
; CHECK-LABEL: postincrement_program_byte:
; CHECK:       ldp8u {{.*}}+
; CHECK:       st16
; CHECK:       st8
  %value = load i8, ptr addrspace(1) %address, align 1
  %next = getelementptr i8, ptr addrspace(1) %address, i32 1
  store ptr addrspace(1) %next, ptr %updated, align 1
  ret i8 %value
}

define i16 @volatile_program_bytes(ptr addrspace(1) %address) {
; CHECK-LABEL: volatile_program_bytes:
; CHECK:       ldp8u
; CHECK:       ldp8u
; CHECK-NOT:   ldp16
  %first = load volatile i8, ptr addrspace(1) %address, align 1
  %next = getelementptr i8, ptr addrspace(1) %address, i32 1
  %second = load volatile i8, ptr addrspace(1) %next, align 1
  %a = zext i8 %first to i16
  %b = zext i8 %second to i16
  %sum = add i16 %a, %b
  ret i16 %sum
}

; CHECK:       .type flash_callback,@object
; CHECK:       .progptr %prog24(callback)
; CHECK:       .size flash_callback, 3
; CHECK:       .type data_callback,@object
; CHECK:       .progptr %prog24(callback)
; CHECK:       .size data_callback, 3
; CHECK:       .type data_callback_array,@object
; CHECK:       .progptr %prog24(callback)
; CHECK-NEXT:  .short 0
; CHECK-NEXT:  .byte 0
; CHECK-NEXT:  .progptr %prog24(callback)
; CHECK:       .size data_callback_array, 9
; CHECK:       .type flash_callback_array,@object
; CHECK:       .progptr %prog24(callback)
; CHECK-NEXT:  .short 0
; CHECK-NEXT:  .byte 0
; CHECK:       .size flash_callback_array, 6
