// RUN: %clang_cc1 -triple avm -fsyntax-only -verify %s
struct aggregate { int value; };
int aggregate_argument(const char *format, struct aggregate value) {
  return __avm_debug_printf(format, value); // expected-error {{argument 2 to AVM variadic formatting builtin has unsupported type}}
}
int builtin_aggregate_argument(const char *format, struct aggregate value) {
  return __builtin_avm_debug_printf(format, value); // expected-error {{argument 2 to AVM variadic formatting builtin has unsupported type}}
}
int wrong_format(const char __attribute__((address_space(1))) *format) {
  return __avm_debug_printf(format); // expected-error {{changes address space of pointer}}
}
int wrong_vformat(const char __attribute__((address_space(1))) *format,
                  __builtin_va_list ap) {
  return __avm_debug_printfv(format, ap); // expected-error {{changes address space of pointer}}
}
