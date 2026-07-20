// RUN: %clang --target=avm-unknown-arduboyfx -ffreestanding -O2 -S -emit-llvm \
// RUN:   %s -o - | FileCheck %s --check-prefix=IR
// RUN: %clang --target=avm-unknown-arduboyfx -ffreestanding -O2 -S \
// RUN:   %s -o - | FileCheck %s --check-prefix=ASM

#include <avm/pgmspace.h>

avm_flash_string_t message = F("Hello");

void *copy_message(void *dst, uint16_t size) {
  return memcpy_P(dst, message, size);
}

void *copy_literal(void *dst) { return memcpy_P(dst, F("Hello"), 6); }

void *(*fallback_address)(void *, avm_progmem_cptr, uint16_t) = &memcpy_P;

// IR: @.avm.flashstr.0 = private addrspace(1) constant [6 x i8] c"Hello\00"
// IR: @message = {{.*}}global ptr addrspace(1) @.avm.flashstr.0
// IR: @fallback_address = {{.*}}global ptr addrspace(1) @memcpy_P
// IR-LABEL: define{{.*}} ptr @copy_message
// IR: call addrspace(1) void @llvm.memcpy.p0.p1.i16
// IR-LABEL: define{{.*}} ptr @copy_literal
// IR: call addrspace(1) void @llvm.memcpy.p0.p1.i16
// IR-NOT: llvm.avm.memcpy.p
// IR-NOT: addrspacecast
// ASM-LABEL: copy_message:
// ASM: sys memcpy_p
// ASM-LABEL: copy_literal:
// ASM-NOT: sys memcpy_p
// ASM: ldp
// ASM: st
