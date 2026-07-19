// RUN: not %clang_cc1 -triple avm -fsyntax-only %s 2>&1 | FileCheck %s

_Float16 value;

// CHECK: error: _Float16 is not supported on this target
