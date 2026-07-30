// RUN: %clang --target=avm-unknown-arduboyfx -ffreestanding -O2 -S -emit-llvm \
// RUN:   %s -o - | FileCheck %s --check-prefix=IR
// RUN: %clang --target=avm-unknown-arduboyfx -ffreestanding -O2 -S \
// RUN:   %s -o - | FileCheck %s --check-prefix=ASM

#include <avm/text.h>

extern const avm_font_t AVM_PROGMEM font;

avm_text_cursor_t header_draw_ram(int16_t x, int16_t y, const char *text) {
  set_text_font(&font);
  set_text_mode(AVM_TEXT_WHITE_TRANSPARENT);
  return draw_text(x, y, text);
}

avm_text_cursor_t header_draw_program(int16_t x, int16_t y) {
  return draw_text_P(x, y, F("hello"));
}

avm_text_cursor_t header_vdraw(int16_t x, int16_t y, const char *format,
                               va_list args) {
  return draw_textfv(x, y, format, args);
}

avm_text_cursor_t header_vdraw_program(int16_t x, int16_t y,
                                       avm_flash_string_t format,
                                       va_list args) {
  return draw_textfv_P(x, y, format, args);
}

// Four-byte aggregates are coerced to i32 by the AVM C ABI.
// IR-LABEL: define{{.*}} i32 @header_draw_ram
// IR: call void @llvm.avm.set.text.font
// IR: call void @llvm.avm.set.text.mode
// IR: call i32 @llvm.avm.draw.text
// IR-LABEL: define{{.*}} i32 @header_draw_program
// IR: call i32 @llvm.avm.draw.text.p
// IR-LABEL: define{{.*}} i32 @header_vdraw
// IR: call i32 @llvm.avm.draw.textfv
// IR-LABEL: define{{.*}} i32 @header_vdraw_program
// IR: call i32 @llvm.avm.draw.textfv.p

// ASM-LABEL: header_draw_ram:
// ASM: sys set_text_font
// ASM: sys set_text_mode
// ASM: sys draw_text
// ASM-LABEL: header_draw_program:
// ASM: sys draw_text_p
// ASM-LABEL: header_vdraw:
// ASM: sys draw_textfv
// ASM-LABEL: header_vdraw_program:
// ASM: sys draw_textfv_p
