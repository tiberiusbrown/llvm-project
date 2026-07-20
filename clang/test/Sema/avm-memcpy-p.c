// RUN: %clang_cc1 -triple avm -fsyntax-only -verify %s

#define AS1 __attribute__((address_space(1)))
typedef const void AS1 *avm_progmem_cptr;
struct item { unsigned short value; };

void test(void *dst, char AS1 *chars, unsigned char AS1 *bytes,
          struct item AS1 *record, void AS1 *opaque, char *data) {
  void *a = __builtin_avm_memcpy_p(dst, chars, 1);
  char *b = __builtin_avm_memcpy_p(dst, bytes, 2);
  struct item *c = __builtin_avm_memcpy_p(dst, record, 3);
  avm_progmem_cptr p = opaque;
  __builtin_avm_memcpy_p(dst, p, 0x10001); // expected-warning {{changes value from 65537 to 1}}
  __builtin_avm_memcpy_p(dst, data, 1); // expected-error {{changes address space of pointer}}
  (void)a; (void)b; (void)c;
}
