// RUN: %clang_cc1 -triple avm -emit-llvm -o - %s | FileCheck %s
// RUN: %clang_cc1 -triple avm -emit-obj -o %t %s
// RUN: llvm-readobj --sections --relocations --symbols %t | FileCheck %s --check-prefix=OBJ
// RUN: ld.lld -e function_use %t -o %t.elf
// RUN: llvm-readobj --relocations --symbols %t.elf \
// RUN:   | FileCheck %s --check-prefix=LINK
// RUN: llvm-objdump -s %t.elf | FileCheck %s --check-prefix=LINK-DATA

#define AS1 __attribute__((address_space(1)))
typedef const char AS1 *flash_ptr;

flash_ptr file_scope = __builtin_avm_flash_string("Hello");
flash_ptr table[] = {__builtin_avm_flash_string("Hello"),
                     __builtin_avm_flash_string("Hell" "o")};
flash_ptr const AS1 flash_table[] = {__builtin_avm_flash_string("Hello")};
const char *ordinary = "Hello";

flash_ptr function_use(void) {
  static flash_ptr local = __builtin_avm_flash_string("Hello");
  static flash_ptr local_table[] = {
      __builtin_avm_flash_string("Hello"),
      __builtin_avm_flash_string("Hell" "o")};
  (void)local_table;
  return __builtin_avm_flash_string("Hello");
}

flash_ptr embedded_null(void) {
  return __builtin_avm_flash_string("Hello\0");
}

// CHECK: @.avm.flashstr.0 = private addrspace(1) constant [6 x i8] c"Hello\00", align 1
// CHECK: @file_scope = global ptr addrspace(1) @.avm.flashstr.0, align 1
// CHECK: @table = global [2 x ptr addrspace(1)] [ptr addrspace(1) @.avm.flashstr.0, ptr addrspace(1) @.avm.flashstr.0], align 1
// CHECK: @flash_table = addrspace(1) constant [1 x ptr addrspace(1)] [ptr addrspace(1) @.avm.flashstr.0], align 1
// CHECK: @.str = private unnamed_addr constant [6 x i8] c"Hello\00", align 1
// CHECK: @.avm.flashstr.1 = private addrspace(1) constant [7 x i8] c"Hello\00\00", align 1
// CHECK-NOT: @.avm.flashstr.2
// CHECK-NOT: addrspacecast
// CHECK-NOT: call ptr addrspace(1)

// OBJ: Section ({{.*}}) .rela.rodata {
// OBJ: R_AVM_PROG24
// OBJ: Section ({{.*}}) .rela.data {
// OBJ: R_AVM_PROG24
// OBJ: Name: file_scope
// OBJ: Section: .data
// OBJ: Name: flash_table
// OBJ: Section: .rodata

// LINK: Relocations [
// LINK-NEXT: ]
// LINK: Name: .L.avm.flashstr.0
// LINK: Value: 0x210
// LINK: Name: file_scope
// LINK: Value: 0x100
// LINK: Name: flash_table
// LINK: Value: 0x216
// LINK-DATA: Contents of section .data:
// LINK-DATA-NEXT: 0100 10020010 02001002
// LINK-DATA: Contents of section .rodata:
// LINK-DATA-NEXT: 0210 48656c6c 6f001002 0048656c 6c6f0000
