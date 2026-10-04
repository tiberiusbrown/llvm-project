// RUN: %clang_cc1 -triple avm -fsyntax-only -verify %s
void nullary(void);
void unary(int);
void test(int kind) {
  __builtin_avm_stack_dispatch(nullary, 0);
  __builtin_avm_stack_dispatch(nullary, 3);
  __builtin_avm_stack_dispatch(unary, 0); // expected-error {{AVM stack dispatch requires}}
  __builtin_avm_stack_dispatch(nullary, kind); // expected-error {{AVM stack dispatch requires}}
  __builtin_avm_stack_dispatch(nullary, -1); // expected-error {{AVM stack dispatch requires}}
  __builtin_avm_stack_dispatch(nullary, 4); // expected-error {{AVM stack dispatch requires}}
}
