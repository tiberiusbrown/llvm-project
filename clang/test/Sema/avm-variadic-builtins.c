// RUN: %clang_cc1 -triple avm-unknown-arduboyfx -ffreestanding -fsyntax-only \
// RUN:   -verify %s

typedef unsigned short uint16_t;
typedef const char __attribute__((address_space(1))) *progmem_char_ptr;

struct aggregate { int value; };

void valid(char *dst, uint16_t size, const char *ram_format,
           progmem_char_ptr program_format, int value, long wide,
           const char *ram_string, progmem_char_ptr program_string) {
  __avm_snprintf(dst, size, ram_format, value, wide, ram_string,
                 program_string);
  __avm_snprintf_P(dst, size, program_format, value);
  __avm_draw_textf(0, 8, ram_format, value);
  __avm_draw_textf_P(0, 8, program_format, value);
}

void invalid(char *dst, uint16_t size, const char *format,
             struct aggregate value, _Complex float complex_value) {
  __avm_snprintf(dst, size, format, value); // expected-error {{argument 4 to AVM variadic formatting builtin has unsupported type 'struct aggregate'}}
  __avm_draw_textf(0, 8, format, complex_value); // expected-error {{argument 4 to AVM variadic formatting builtin has unsupported type '_Complex .*'}}
}
