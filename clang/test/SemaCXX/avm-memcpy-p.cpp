// RUN: %clang_cc1 -triple avm -std=c++17 -fsyntax-only -verify %s

#define AS1 __attribute__((address_space(1)))
struct item { unsigned short value; };

void test(void *dst, char AS1 *chars, unsigned char AS1 *bytes,
          item AS1 *record, void AS1 *opaque, char *data) {
  void *a = __builtin_avm_memcpy_p(dst, chars, 1);
  char *b = static_cast<char *>(__builtin_avm_memcpy_p(dst, bytes, 2));
  item *c = static_cast<item *>(__builtin_avm_memcpy_p(dst, record, 3));
  __builtin_avm_memcpy_p(dst, opaque, 4);
  __builtin_avm_memcpy_p(dst, data, 1); // expected-error {{changes address space of pointer}}
  (void)a; (void)b; (void)c;
}
