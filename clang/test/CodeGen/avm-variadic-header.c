// RUN: %clang --target=avm-unknown-arduboyfx -ffreestanding -O2 -S -emit-llvm \
// RUN:   %s -o - | FileCheck %s --check-prefix=IR
// RUN: %clang --target=avm-unknown-arduboyfx -ffreestanding -O2 -S \
// RUN:   %s -o - | FileCheck %s --check-prefix=ASM

#include <avm/text.h>

avm_text_cursor_t header_drawf(int16_t x, int16_t y, unsigned value) {
  return draw_textf(x, y, "%u", value);
}

avm_text_cursor_t header_drawf_program(int16_t x, int16_t y, unsigned value) {
  return draw_textf_P(x, y, F("%u"), value);
}

// The four-byte cursor aggregate is ABI-coerced to i32.
// IR-LABEL: define{{.*}} i32 @header_drawf
// IR: call i32 @llvm.avm.draw.textfv
// IR-LABEL: define{{.*}} i32 @header_drawf_program
// IR: call i32 @llvm.avm.draw.textfv.p

// ASM-LABEL: header_drawf:
// ASM-NOT: call
// ASM: sys draw_textfv
// ASM-LABEL: header_drawf_program:
// ASM-NOT: call
// ASM: sys draw_textfv_p
