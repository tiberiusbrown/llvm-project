// RUN: not %clang --target=avm -std=c++17 -O0 -fsyntax-only %s 2>&1 \
// RUN:   | FileCheck %s
// RUN: not %clang --target=avm -std=c++17 -O2 -fsyntax-only %s 2>&1 \
// RUN:   | FileCheck %s

#define AS1 __attribute__((address_space(1)))
using data_ptr = int *;
using program_ptr = int AS1 *;

void invalid(data_ptr data, program_ptr program, bool choose) {
  data_ptr bad_data = program;
  program_ptr bad_program = data;
  bad_data = static_cast<data_ptr>(program);
  bad_program = reinterpret_cast<program_ptr>(data);
  (void)(choose ? data : program);
  *program = 1;
}

// CHECK: error: initializing 'data_ptr'
// CHECK: error: initializing 'program_ptr'
// CHECK: error: static_cast from 'program_ptr'
// CHECK: error: reinterpret_cast from 'data_ptr'
// CHECK: error: incompatible operand types ('data_ptr' (aka 'int *') and 'program_ptr'
// CHECK: error: address space 1 is read-only
