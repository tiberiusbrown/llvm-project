// RUN: %clang_cc1 -triple avm -fsyntax-only -verify %s
void invalid(void) {
  __builtin_avm_termination_context(); // expected-error {{AVM termination context requires the raw entry function '_start'}}
}
void _start(void) { __builtin_avm_termination_context(); }
