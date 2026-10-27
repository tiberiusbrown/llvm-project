// RUN: %clang_cc1 -triple avm -emit-llvm -o - %s | FileCheck %s
// CHECK: alloca i16, align 1, !avm.termination.phase
// CHECK: store volatile i16 0
void _start(void) { __builtin_avm_termination_context(); }
