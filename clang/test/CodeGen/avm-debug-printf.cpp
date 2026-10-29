// RUN: %clang_cc1 -triple avm -emit-llvm -O0 -o - %s | FileCheck %S/avm-debug-printf.c
extern "C" {
#include "avm-debug-printf.c"
}
