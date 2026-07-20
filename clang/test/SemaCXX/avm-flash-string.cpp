// RUN: %clang_cc1 -triple avm -std=c++20 -fsyntax-only -verify %s
// RUN: %clang_cc1 -triple avm -std=c++20 -fexperimental-new-constant-interpreter -fsyntax-only -verify %s

#define AS1 __attribute__((address_space(1)))
using flash_ptr = const char AS1 *;
struct holder { flash_ptr value; };

constexpr flash_ptr file_scope = __builtin_avm_flash_string("Hello");
constinit flash_ptr initialized = __builtin_avm_flash_string("Hell" "o");
constexpr holder aggregate = {__builtin_avm_flash_string("aggregate")};
constexpr int select(flash_ptr) { return 1; }
constexpr int select(const char *) { return 2; }
static_assert(select(__builtin_avm_flash_string("overload")) == 1);

void valid() {
  static constexpr flash_ptr local = __builtin_avm_flash_string("local");
  (void)local;
}

void invalid(const char *variable, bool condition) {
  __builtin_avm_flash_string(variable); // expected-error {{argument to '__builtin_avm_flash_string' must be an ordinary narrow string literal}}
  __builtin_avm_flash_string(condition ? "a" : "b"); // expected-error {{argument to '__builtin_avm_flash_string' must be an ordinary narrow string literal}}
  __builtin_avm_flash_string((char[2]){'x', 0}); // expected-error {{argument to '__builtin_avm_flash_string' must be an ordinary narrow string literal}}
  __builtin_avm_flash_string(L"wide"); // expected-error {{argument to '__builtin_avm_flash_string' must be an ordinary narrow string literal}}
  __builtin_avm_flash_string(u8"utf8"); // expected-error {{argument to '__builtin_avm_flash_string' must be an ordinary narrow string literal}}
  __builtin_avm_flash_string(u"utf16"); // expected-error {{argument to '__builtin_avm_flash_string' must be an ordinary narrow string literal}}
  __builtin_avm_flash_string(U"utf32"); // expected-error {{argument to '__builtin_avm_flash_string' must be an ordinary narrow string literal}}
  const char *ordinary = __builtin_avm_flash_string("bad"); // expected-error {{changes address space of pointer}}
}
