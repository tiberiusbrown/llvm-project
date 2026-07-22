// RUN: %clang_cc1 -triple avm -fsyntax-only -verify %s

typedef unsigned int uint16_t;
typedef const void __attribute__((address_space(1))) *progmem_void_ptr;
typedef const char __attribute__((address_space(1))) *progmem_char_ptr;

void accepted(char *dst, const char *data, progmem_void_ptr pvoid,
              progmem_char_ptr pchar, uint16_t n) {
  int a = __builtin_avm_memcmp_p(data, pvoid, n);
  int b = __avm_strcmp_P(data, pchar);
  uint16_t c = strlen_P(pchar);
  char *d = __builtin_avm_strncpy_p(dst, pchar, n);
  char *e = strncat_P(dst, pchar, n);
  int f = __avm_memcmp(data, data, n);
  int g = __avm_strcmp(data, data);
  uint16_t h = __avm_strlen(data);
  char *i = __avm_strncpy(dst, data, n);
  char *j = __avm_strncat(dst, data, n);
  (void)a; (void)b; (void)c; (void)d; (void)e;
  (void)f; (void)g; (void)h; (void)i; (void)j;
}

_Static_assert(__builtin_types_compatible_p(
                   __typeof__(__builtin_avm_strlen_p((progmem_char_ptr)0)),
                   uint16_t), "strlen_P result");
_Static_assert(__builtin_types_compatible_p(
                   __typeof__(__builtin_avm_memcmp_p(
                       (const void *)0, (progmem_void_ptr)0, 0)), int),
               "memcmp_P result");
_Static_assert(__builtin_types_compatible_p(
                   __typeof__(__builtin_avm_strncpy_p(
                       (char *)0, (progmem_char_ptr)0, 0)), char *),
               "strncpy_P result");

void rejected(char *data, progmem_char_ptr program, uint16_t n) {
  __builtin_avm_memcmp_p(data, data, n); // expected-error {{changes address space of pointer}}
  __avm_strcmp_P(data, data); // expected-error {{changes address space of pointer}}
  __builtin_avm_strlen_p(data); // expected-error {{changes address space of pointer}}
  __avm_strncpy_P(data, data, n); // expected-error {{changes address space of pointer}}
  strncat_P(data, data, n); // expected-error {{changes address space of pointer}}

  __avm_memcmp(data, program, n); // expected-error {{changes address space of pointer}}
  __avm_strcmp(data, program); // expected-error {{changes address space of pointer}}
  __avm_strlen(program); // expected-error {{changes address space of pointer}}
  __avm_strncpy(data, program, n); // expected-error {{changes address space of pointer}}
  __avm_strncat(data, program, n); // expected-error {{changes address space of pointer}}

  __builtin_avm_memcmp_p(data, program); // expected-error {{too few arguments}}
  __avm_strcmp(data); // expected-error {{too few arguments}}
  __builtin_avm_strlen_p(program, program); // expected-error {{too many arguments}}
  __avm_strncpy(data, data); // expected-error {{too few arguments}}
  strncat_P(data, program, n, n); // expected-error {{too many arguments}}
}
