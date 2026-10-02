// RUN: %clang --target=avm-unknown-arduboyfx -nostdlibinc -I%S/../../../../../runtime/include \
// RUN:   -ffreestanding -std=c++20 -fsyntax-only -Xclang -verify \
// RUN:   -Xclang -verify-ignore-unexpected=note %s

#include <string.h>

void test(char *dst) {
  const char *ram = nullptr;
  const char AVM_PROGMEM *flash = nullptr;
  (void)strlen(ram);
  (void)strlen(flash);
  (void)memcpy(dst, ram, 1);
  (void)memcpy(dst, flash, 1);
  (void)strlen(nullptr); // expected-error {{call to '__avm_cpp_strlen' is ambiguous}}
  (void)memcpy(dst, nullptr, 1); // expected-error {{call to '__avm_cpp_memcpy' is ambiguous}}
}
