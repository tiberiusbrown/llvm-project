// RUN: %clang_cc1 -triple avm-unknown-arduboyfx -std=c++23 -O0 -disable-llvm-verifier -emit-obj -o %t %s
// RUN: %clang_cc1 -triple avm-unknown-arduboyfx -std=c++23 -O0 -emit-llvm -o - %s | FileCheck %s

template <typename T, unsigned N>
bool equal(const T *actual, const T (&expected)[N], unsigned size) {
  if (size != N)
    return false;
  for (unsigned i = 0; i < N; ++i)
    if (actual[i] != expected[i])
      return false;
  return true;
}

template bool equal(const int *, const int (&)[4], unsigned);

// CHECK-LABEL: define {{.*}} @_Z5equalIiLj4EE
// CHECK: icmp ne i16 {{%.*}}, 4
// CHECK: icmp ult i16 {{%.*}}, 4

using FlashChar = const __attribute__((address_space(1))) char;

template <unsigned long N>
bool matches_large_bound(FlashChar (&)[N], unsigned long size) {
  return size == N;
}

template bool matches_large_bound(FlashChar (&)[70000], unsigned long);

// CHECK-LABEL: define {{.*}} @_Z19matches_large_boundILm70000EE
// CHECK: icmp eq i32 {{%.*}}, 70000
