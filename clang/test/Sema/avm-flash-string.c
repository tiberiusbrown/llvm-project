// RUN: %clang_cc1 -triple avm -fsyntax-only -verify %s

#define AS1 __attribute__((address_space(1)))
typedef const char AS1 *flash_ptr;
struct holder { flash_ptr value; };

flash_ptr file_scope = __builtin_avm_flash_string("Hello");
struct holder aggregate = {__builtin_avm_flash_string("Hell" "o")};
_Static_assert(_Generic(__builtin_avm_flash_string("x"), flash_ptr : 1,
                        default : 0),
               "exact flash pointer type");

void valid(void) {
  static flash_ptr local = __builtin_avm_flash_string("local");
  static struct holder local_aggregate = {
      __builtin_avm_flash_string("local aggregate")};
  flash_ptr concat = __builtin_avm_flash_string(("a" "b"));
  (void)local;
  (void)local_aggregate;
  (void)concat;
}

void invalid(const char *variable, int condition) {
  __builtin_avm_flash_string(variable); // expected-error {{argument to '__builtin_avm_flash_string' must be an ordinary narrow string literal}}
  __builtin_avm_flash_string(condition ? "a" : "b"); // expected-error {{argument to '__builtin_avm_flash_string' must be an ordinary narrow string literal}}
  __builtin_avm_flash_string((char[]){'x', 0}); // expected-error {{argument to '__builtin_avm_flash_string' must be an ordinary narrow string literal}}
  __builtin_avm_flash_string(L"wide"); // expected-error {{argument to '__builtin_avm_flash_string' must be an ordinary narrow string literal}}
  __builtin_avm_flash_string(u8"utf8"); // expected-error {{argument to '__builtin_avm_flash_string' must be an ordinary narrow string literal}}
  __builtin_avm_flash_string(u"utf16"); // expected-error {{argument to '__builtin_avm_flash_string' must be an ordinary narrow string literal}}
  __builtin_avm_flash_string(U"utf32"); // expected-error {{argument to '__builtin_avm_flash_string' must be an ordinary narrow string literal}}
  const char *ordinary = __builtin_avm_flash_string("bad"); // expected-error {{changes address space of pointer}}
}
