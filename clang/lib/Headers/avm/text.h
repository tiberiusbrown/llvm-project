//===--- text.h - AVM font and text drawing API ---------------------------===//

#ifndef __AVM_TEXT_H
#define __AVM_TEXT_H

#include <stdarg.h>
#include <stdint.h>
#include <avm/pgmspace.h>

typedef struct __attribute__((packed)) {
  uint8_t w;
  uint8_t h;
  int8_t xoff;
  int8_t yoff;
  uint8_t xadv;
  uint8_t image_offset[3];
} avm_font_glyph_t;

// The packed program-space object contains this three-byte header, followed by
// num_glyphs avm_font_glyph_t records and then the page-major glyph images.
typedef struct __attribute__((packed)) {
  uint8_t line_height;
  uint8_t glyph_first;
  uint8_t num_glyphs;
} avm_font_t;

typedef enum {
  AVM_TEXT_OVERWRITE = 0,
  AVM_TEXT_WHITE_TRANSPARENT = 2,
  AVM_TEXT_BLACK_TRANSPARENT = 3,
} avm_text_mode_t;

typedef struct {
  int16_t x;
  int16_t baseline_y;
} avm_text_cursor_t;

static __inline__ avm_text_cursor_t
__avm_text_cursor_from_u32(uint32_t packed) {
  avm_text_cursor_t result;
  result.x = (int16_t)(uint16_t)packed;
  result.baseline_y = (int16_t)(uint16_t)(packed >> 16);
  return result;
}

static __inline__ void
set_text_font(const avm_font_t AVM_PROGMEM *font) {
  __avm_set_text_font(font);
}

static __inline__ void set_text_mode(avm_text_mode_t mode) {
  __avm_set_text_mode((uint8_t)mode);
}

static __inline__ avm_text_cursor_t
draw_text(int16_t x, int16_t baseline_y, const char *string) {
  return __avm_text_cursor_from_u32(
      __avm_draw_text(x, baseline_y, string));
}

static __inline__ avm_text_cursor_t
draw_text_P(int16_t x, int16_t baseline_y, avm_flash_string_t string) {
  return __avm_text_cursor_from_u32(
      __avm_draw_text_P(x, baseline_y, string));
}

static __inline__ avm_text_cursor_t
draw_textfv(int16_t x, int16_t baseline_y, const char *format, va_list args) {
  return __avm_text_cursor_from_u32(
      __avm_draw_textfv(x, baseline_y, format, args));
}

static __inline__ avm_text_cursor_t
draw_textfv_P(int16_t x, int16_t baseline_y, avm_flash_string_t format,
              va_list args) {
  return __avm_text_cursor_from_u32(
      __avm_draw_textfv_P(x, baseline_y, format, args));
}

// A C function cannot forward its unnamed arguments without constructing a
// va_list. These macros retain the cursor-valued API while allowing Clang to
// expand the true-variadic builtin directly at the caller.
#define draw_textf(...)                                                       \
  __avm_text_cursor_from_u32(__avm_draw_textf(__VA_ARGS__))
#define draw_textf_P(...)                                                     \
  __avm_text_cursor_from_u32(__avm_draw_textf_P(__VA_ARGS__))

#endif // __AVM_TEXT_H
