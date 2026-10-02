// RUN: %clang_cc1 -triple avm -emit-llvm -o - %s | FileCheck %s

#define AS1 __attribute__((address_space(1)))

// Public _P names are ordinary functions, not AVM target builtins.
void *memcpy_P(void *d, const void AS1 *s, unsigned n) {
  return __avm_memcpy_P(d, s, n);
}
int memcmp_P(const void *a, const void AS1 *b, unsigned n) {
  return __avm_memcmp_P(a, b, n);
}
int strcmp_P(const char *a, const char AS1 *b) {
  return __avm_strcmp_P(a, b);
}
unsigned strlen_P(const char AS1 *s) { return __avm_strlen_P(s); }
char *strncpy_P(char *d, const char AS1 *s, unsigned n) {
  return __avm_strncpy_P(d, s, n);
}
char *strncat_P(char *d, const char AS1 *s, unsigned n) {
  return __avm_strncat_P(d, s, n);
}

// CHECK-DAG: define{{.*}} @memcpy_P(
// CHECK-DAG: define{{.*}} @memcmp_P(
// CHECK-DAG: define{{.*}} @strcmp_P(
// CHECK-DAG: define{{.*}} @strlen_P(
// CHECK-DAG: define{{.*}} @strncpy_P(
// CHECK-DAG: define{{.*}} @strncat_P(
