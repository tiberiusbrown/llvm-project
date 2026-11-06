// RUN: %clang_cc1 -triple avm -Werror -emit-llvm -o - %s | FileCheck %s

typedef unsigned short (__attribute__((preserve_none)) *callback)(unsigned short);

// CHECK-LABEL: define {{.*}}preserve_nonecc i16 @callee(i16 noundef{{.*}}%value)
__attribute__((preserve_none)) unsigned short callee(unsigned short value) {
  return value + 1;
}

// CHECK-LABEL: define {{.*}}i16 @indirect(
// CHECK: call preserve_nonecc addrspace(1) i16 %{{.*}}(i16 noundef{{.*}})
unsigned short indirect(callback function, unsigned short value) {
  return function(value);
}

// CHECK-LABEL: define {{.*}}i16 @direct(
// CHECK: call preserve_nonecc addrspace(1) i16 @callee(i16 noundef{{.*}})
unsigned short direct(unsigned short value) {
  return callee(value);
}
