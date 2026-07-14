// RUN: %clang_cc1 -triple abc -emit-llvm -o - %s | FileCheck %s

typedef unsigned char u8;

extern int first(void);
extern int second(void);
extern int third(void);
extern void consume(int, int, int);
extern void variadic(int, ...);
extern u8 buttons(void) asm("$buttons");
extern void debug_printf(const char __attribute__((address_space(1))) *, ...)
    asm("$debug_printf");

void ordinary(void) {
  consume(first(), second(), third());
}

// CHECK-LABEL: define{{.*}} @ordinary
// CHECK: call{{.*}} @third
// CHECK-NEXT: call{{.*}} @second
// CHECK-NEXT: call{{.*}} @first
// CHECK: call void @consume

void varargs(void) {
  variadic(first(), second(), third());
}

// CHECK-LABEL: define{{.*}} @varargs
// CHECK: call{{.*}} @third
// CHECK-NEXT: call{{.*}} @second
// CHECK-NEXT: call{{.*}} @first
// CHECK: call void (i16, ...) @variadic

void syscalls(const char __attribute__((address_space(1))) *fmt) {
  debug_printf(fmt, buttons(), buttons());
}

// CHECK-LABEL: define{{.*}} @syscalls
// CHECK: call{{.*}} @"$buttons"
// CHECK-NEXT: call{{.*}} @"$buttons"
// CHECK: call void (ptr addrspace(1), ...) @"$debug_printf"
