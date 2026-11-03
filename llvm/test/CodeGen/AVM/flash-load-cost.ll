; RUN: opt -mtriple=avm-unknown-arduboyfx -mcpu=avm1 \
; RUN:   -mtune=avm-interpreter-32u4-v1 -passes="print<cost-model>" \
; RUN:   -cost-kind=throughput -disable-output 2>&1 < %s | FileCheck %s
; RUN: opt -mtriple=avm-unknown-arduboyfx -mcpu=avm1 \
; RUN:   -mtune=avm-interpreter-32u4-v1 -passes="print<cost-model>" \
; RUN:   -cost-kind=code-size -disable-output 2>&1 < %s \
; RUN:   | FileCheck %s --check-prefix=SIZE

; An AS0 load costs its flash-window measurement only when the object it reads
; is placed in the flash window. Other AS0 loads keep their RAM costs, and program
; loads keep their program-space costs. Code size counts the same instruction bytes
; for every AS0 load.

@flash16 = private constant [4 x i16] [i16 1, i16 2, i16 3, i16 4]
@flash8 = private constant [4 x i8] c"\01\02\03\04"
@flash32 = private constant [2 x i32] [i32 1, i32 2]
@scalar16 = internal constant i16 9
@scalar16.alias = internal alias i16, ptr @scalar16
@ram16 = global [4 x i16] zeroinitializer
@user.table = constant [4 x i16] [i16 5, i16 6, i16 7, i16 8], section "user.table"
@explicit.flash = constant [4 x i16] [i16 9, i16 10, i16 11, i16 12], section ".flash.explicit"

define i16 @gep_flash_load(i16 %i) {
; CHECK-LABEL: Cost Model: Found an estimated cost of 15 for instruction: %v = load i16, ptr %p, align 2
; SIZE-LABEL: Cost Model: Found an estimated cost of 1 for instruction: %v = load i16, ptr %p, align 2
  %p = getelementptr inbounds [4 x i16], ptr @flash16, i16 0, i16 %i
  %v = load i16, ptr %p, align 2
  ret i16 %v
}

define i8 @direct_flash_byte() {
; CHECK-LABEL: Cost Model: Found an estimated cost of 14 for instruction: %v = load i8, ptr @flash8, align 1
; SIZE-LABEL: Cost Model: Found an estimated cost of 1 for instruction: %v = load i8, ptr @flash8, align 1
  %v = load i8, ptr @flash8, align 1
  ret i8 %v
}

define i32 @flash_wide_load() {
; CHECK-LABEL: Cost Model: Found an estimated cost of 19 for instruction: %v = load i32, ptr @flash32, align 4
; SIZE-LABEL: Cost Model: Found an estimated cost of 3 for instruction: %v = load i32, ptr @flash32, align 4
  %v = load i32, ptr @flash32, align 4
  ret i32 %v
}

define i16 @alias_flash_load() {
; CHECK-LABEL: Cost Model: Found an estimated cost of 15 for instruction: %v = load i16, ptr @scalar16.alias, align 2
; SIZE-LABEL: Cost Model: Found an estimated cost of 1 for instruction: %v = load i16, ptr @scalar16.alias, align 2
  %v = load i16, ptr @scalar16.alias, align 2
  ret i16 %v
}

define i16 @writable_global_load(i16 %i) {
; CHECK-LABEL: Cost Model: Found an estimated cost of 1 for instruction: %v = load i16, ptr %p, align 2
  %p = getelementptr inbounds [4 x i16], ptr @ram16, i16 0, i16 %i
  %v = load i16, ptr %p, align 2
  ret i16 %v
}

define i16 @alloca_load(i16 %x) {
; CHECK-LABEL: Cost Model: Found an estimated cost of 1 for instruction: %v = load i16, ptr %buf, align 2
  %buf = alloca i16, align 2
  store i16 %x, ptr %buf, align 2
  %v = load i16, ptr %buf, align 2
  ret i16 %v
}

define i16 @argument_load(ptr %p) {
; CHECK-LABEL: Cost Model: Found an estimated cost of 1 for instruction: %v = load i16, ptr %p, align 2
  %v = load i16, ptr %p, align 2
  ret i16 %v
}

define i16 @explicit_section_load(i16 %i) {
; CHECK-LABEL: Cost Model: Found an estimated cost of 1 for instruction: %v = load i16, ptr %p, align 2
  %p = getelementptr inbounds [4 x i16], ptr @user.table, i16 0, i16 %i
  %v = load i16, ptr %p, align 2
  ret i16 %v
}

define i16 @explicit_flash_load(i16 %i) {
; CHECK-LABEL: Cost Model: Found an estimated cost of 15 for instruction: %v = load i16, ptr %p, align 2
  %p = getelementptr inbounds [4 x i16], ptr @explicit.flash, i16 0, i16 %i
  %v = load i16, ptr %p, align 2
  ret i16 %v
}

define i16 @punned_flash_load() {
; CHECK-LABEL: Cost Model: Found an estimated cost of 15 for instruction: %v = load i16, ptr %p, align 1
; SIZE-LABEL: Cost Model: Found an estimated cost of 1 for instruction: %v = load i16, ptr %p, align 1
  %p = getelementptr inbounds i8, ptr @flash8, i16 2
  %v = load i16, ptr %p, align 1
  ret i16 %v
}

define i16 @program_load(ptr addrspace(1) %p) {
; CHECK-LABEL: Cost Model: Found an estimated cost of 18 for instruction: %v = load i16, ptr addrspace(1) %p, align 2
; SIZE-LABEL: Cost Model: Found an estimated cost of 3 for instruction: %v = load i16, ptr addrspace(1) %p, align 2
  %v = load i16, ptr addrspace(1) %p, align 2
  ret i16 %v
}

; A loaded pointer is priced at the width of the address space it points into. An
; AS0 data pointer is 16 bits and costs as an i16 load. An AS1 program pointer is
; 24 bits, and an AS0 load of one is the two-load expansion of LOAD24_PSEUDO: a
; 16-bit postincrement load and an 8-bit load. A flash source prices both loads at
; their flash-window measurements, 282 and 232 cycles (514 total, 30 normalized). A
; RAM source prices them at the RAM measurements, 46 and 20 cycles (66 total, 4
; normalized). Code size is the size of F7LD16_POST and LD8U, 3 bytes, for either
; source. A program pointer loaded from AS1 keeps its LDP24 cost.

@prog_target = addrspace(1) constant [1 x i8] c"\01"
@flash_data_pointer = internal constant ptr @ram16
@flash_program_pointer = internal constant ptr addrspace(1) @prog_target
@flash_program_pointer.alias = internal alias ptr addrspace(1), ptr @flash_program_pointer
@flash_program_table = internal constant [2 x ptr addrspace(1)] [ptr addrspace(1) @prog_target, ptr addrspace(1) @prog_target]
@ram_data_pointer = global ptr @ram16
@ram_program_pointer = global ptr addrspace(1) @prog_target

define ptr @flash_data_pointer_load() {
; CHECK-LABEL: Cost Model: Found an estimated cost of 15 for instruction: %v = load ptr, ptr @flash_data_pointer, align 2
; SIZE-LABEL: Cost Model: Found an estimated cost of 1 for instruction: %v = load ptr, ptr @flash_data_pointer, align 2
  %v = load ptr, ptr @flash_data_pointer, align 2
  ret ptr %v
}

define ptr addrspace(1) @flash_program_pointer_load() {
; CHECK-LABEL: Cost Model: Found an estimated cost of 30 for instruction: %v = load ptr addrspace(1), ptr @flash_program_pointer, align 2
; SIZE-LABEL: Cost Model: Found an estimated cost of 3 for instruction: %v = load ptr addrspace(1), ptr @flash_program_pointer, align 2
  %v = load ptr addrspace(1), ptr @flash_program_pointer, align 2
  ret ptr addrspace(1) %v
}

define ptr addrspace(1) @flash_program_alias_load() {
; CHECK-LABEL: Cost Model: Found an estimated cost of 30 for instruction: %v = load ptr addrspace(1), ptr @flash_program_pointer.alias, align 2
; SIZE-LABEL: Cost Model: Found an estimated cost of 3 for instruction: %v = load ptr addrspace(1), ptr @flash_program_pointer.alias, align 2
  %v = load ptr addrspace(1), ptr @flash_program_pointer.alias, align 2
  ret ptr addrspace(1) %v
}

define ptr addrspace(1) @flash_program_table_load(i16 %i) {
; CHECK-LABEL: Cost Model: Found an estimated cost of 30 for instruction: %v = load ptr addrspace(1), ptr %p, align 2
; SIZE-LABEL: Cost Model: Found an estimated cost of 3 for instruction: %v = load ptr addrspace(1), ptr %p, align 2
  %p = getelementptr inbounds [2 x ptr addrspace(1)], ptr @flash_program_table, i16 0, i16 %i
  %v = load ptr addrspace(1), ptr %p, align 2
  ret ptr addrspace(1) %v
}

define ptr @ram_data_pointer_load() {
; CHECK-LABEL: Cost Model: Found an estimated cost of 1 for instruction: %v = load ptr, ptr @ram_data_pointer, align 2
; SIZE-LABEL: Cost Model: Found an estimated cost of 1 for instruction: %v = load ptr, ptr @ram_data_pointer, align 2
  %v = load ptr, ptr @ram_data_pointer, align 2
  ret ptr %v
}

define ptr addrspace(1) @ram_program_pointer_load() {
; CHECK-LABEL: Cost Model: Found an estimated cost of 4 for instruction: %v = load ptr addrspace(1), ptr @ram_program_pointer, align 2
; SIZE-LABEL: Cost Model: Found an estimated cost of 3 for instruction: %v = load ptr addrspace(1), ptr @ram_program_pointer, align 2
  %v = load ptr addrspace(1), ptr @ram_program_pointer, align 2
  ret ptr addrspace(1) %v
}

define ptr @argument_data_pointer_load(ptr %p) {
; CHECK-LABEL: Cost Model: Found an estimated cost of 1 for instruction: %v = load ptr, ptr %p, align 2
; SIZE-LABEL: Cost Model: Found an estimated cost of 1 for instruction: %v = load ptr, ptr %p, align 2
  %v = load ptr, ptr %p, align 2
  ret ptr %v
}

define ptr addrspace(1) @argument_program_pointer_load(ptr %p) {
; CHECK-LABEL: Cost Model: Found an estimated cost of 4 for instruction: %v = load ptr addrspace(1), ptr %p, align 2
; SIZE-LABEL: Cost Model: Found an estimated cost of 3 for instruction: %v = load ptr addrspace(1), ptr %p, align 2
  %v = load ptr addrspace(1), ptr %p, align 2
  ret ptr addrspace(1) %v
}

define ptr addrspace(1) @program_pointer_from_program_load(ptr addrspace(1) %p) {
; CHECK-LABEL: Cost Model: Found an estimated cost of 19 for instruction: %v = load ptr addrspace(1), ptr addrspace(1) %p, align 2
; SIZE-LABEL: Cost Model: Found an estimated cost of 3 for instruction: %v = load ptr addrspace(1), ptr addrspace(1) %p, align 2
  %v = load ptr addrspace(1), ptr addrspace(1) %p, align 2
  ret ptr addrspace(1) %v
}
