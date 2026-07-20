// RUN: not %clang --target=avm -O0 -fsyntax-only %s 2>&1 \
// RUN:   | FileCheck %s
// RUN: not %clang --target=avm -O2 -fsyntax-only %s 2>&1 \
// RUN:   | FileCheck %s

#define AS1 __attribute__((address_space(1)))
typedef int *data_ptr;
typedef int AS1 *program_ptr;

void invalid(data_ptr data, program_ptr program, int choose) {
  data_ptr bad_data = program;
  program_ptr bad_program = data;
  bad_data = (data_ptr)program;
  bad_program = (program_ptr)data;
  (void)(choose ? data : program);
  *program = 1;
}

// CHECK: error: initializing 'data_ptr' (aka 'int *') with an expression of type 'program_ptr'
// CHECK: error: initializing 'program_ptr'
// CHECK: error: C-style cast from 'program_ptr'
// CHECK: error: C-style cast from 'data_ptr'
// CHECK: error: conditional operator
// CHECK: error: address space 1 is read-only
