// RUN: %clang --target=avm-unknown-arduboyfx -nostdlibinc -I%S/../../../../../runtime/include \
// RUN:   -ffreestanding -std=c++20 -O0 -S -emit-llvm %s -o - | FileCheck %s --check-prefix=IR
// RUN: %clang --target=avm-unknown-arduboyfx -nostdlibinc -I%S/../../../../../runtime/include \
// RUN:   -ffreestanding -std=c++20 -O2 -S %s -o - | FileCheck %s --check-prefix=ASM

#include <avm.h>
#include <stdio.h>
#include <string.h>

void *unified_copy(void *d, const void AVM_PROGMEM *s, size_t n) {
  return memcpy(d, s, n);
}
void *explicit_copy(void *d, const void AVM_PROGMEM *s, size_t n) {
  return memcpy_P(d, s, n);
}
size_t unified_length(const char AVM_PROGMEM *s) { return strlen(s); }
size_t explicit_length(const char AVM_PROGMEM *s) { return strlen_P(s); }
avm_text_cursor_t unified_draw(const char AVM_PROGMEM *s) {
  return avm_draw_text(1, 2, s);
}
avm_text_cursor_t explicit_draw(const char AVM_PROGMEM *s) {
  return avm_draw_text_P(1, 2, s);
}
avm_text_cursor_t unified_drawf(const char AVM_PROGMEM *s, int n) {
  return avm_draw_textf(1, 2, s, n);
}
avm_text_cursor_t explicit_drawf(const char AVM_PROGMEM *s, int n) {
  return avm_draw_textf_P(1, 2, s, n);
}
int unified_snprintf(char *d, size_t n, const char AVM_PROGMEM *s, int x) {
  return snprintf(d, n, s, x);
}
int explicit_snprintf(char *d, size_t n, const char AVM_PROGMEM *s, int x) {
  return snprintf_P(d, n, s, x);
}

// IR-COUNT-2: call addrspace(1) void @llvm.memcpy.p0.p1.i16
// IR-COUNT-2: call{{.*}} @llvm.avm.strlen.p
// IR-COUNT-2: call{{.*}} @llvm.avm.draw.text.p
// IR-COUNT-2: call{{.*}} @llvm.avm.draw.textfv.p
// IR-COUNT-2: call{{.*}} @llvm.avm.vsnprintf.p
// ASM-COUNT-2: sys memcpy_p
// ASM-COUNT-2: sys strlen_p
// ASM-COUNT-2: sys draw_text_p
// ASM-COUNT-2: sys draw_textfv_p
// ASM-COUNT-2: sys vsnprintf_p
// ASM-NOT: {{^[ \t]*call}}
