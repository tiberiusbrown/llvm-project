// RUN: %clang --target=avm-unknown-arduboyfx -nostdlibinc -I%S/../../../../../runtime/include -x c++ -std=c++20 \
// RUN:   -ffreestanding -O2 -S -emit-llvm %s -o - | FileCheck %s
// RUN: %clang --target=avm-unknown-arduboyfx -nostdlibinc -I%S/../../../../../runtime/include -x c++ -std=c++20 \
// RUN:   -fexperimental-new-constant-interpreter -ffreestanding -O2 \
// RUN:   -S -emit-llvm %s -o - | FileCheck %s

#include <avm/pgmspace.h>

constexpr const char AVM_PROGMEM *message = F("Hello");
constinit const char AVM_PROGMEM *second = F("Hello");

void consume(const char AVM_PROGMEM *);
void consume(const char *);

void use_message() { consume(F("Hello")); }

void *copy_message(void *dst, uint16_t size) {
  return memcpy_P(dst, F("Hello"), size);
}

void *copy_literal(void *dst) { return memcpy_P(dst, F("Hello"), 6); }

auto fallback_address = &memcpy_P;

// CHECK: @.avm.flashstr.0 = private addrspace(1) constant [6 x i8] c"Hello\00"
// CHECK-NOT: @.avm.flashstr.1
// CHECK-NOT: llvm.global_ctors
// CHECK: @fallback_address = {{.*}}global ptr addrspace(1) @memcpy_P
// CHECK-COUNT-2: call addrspace(1) void @llvm.memcpy.p0.p1.i16
// CHECK-NOT: addrspacecast
