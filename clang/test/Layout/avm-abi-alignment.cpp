// RUN: %clang_cc1 -triple avm-unknown-arduboyfx -std=c++11 -fsyntax-only %s
// RUN: %clang_cc1 -triple avm-unknown-arduboyfx -std=c++11 -fsyntax-only -fdump-record-layouts %s | FileCheck %s

static_assert(alignof(bool) == 1, "bool ABI alignment");
static_assert(alignof(short) == 1, "short ABI alignment");
static_assert(alignof(int) == 1, "int ABI alignment");
static_assert(alignof(long) == 1, "long ABI alignment");
static_assert(alignof(long long) == 1, "long long ABI alignment");
static_assert(alignof(float) == 1, "float ABI alignment");
static_assert(alignof(double) == 1, "double ABI alignment");
static_assert(alignof(long double) == 1, "long double ABI alignment");
static_assert(alignof(void *) == 1, "data-pointer ABI alignment");

struct S {
  char a;
  long long b;
  char c;
};

static_assert(__builtin_offsetof(S, a) == 0, "S::a offset");
static_assert(__builtin_offsetof(S, b) == 1, "S::b offset");
static_assert(__builtin_offsetof(S, c) == 9, "S::c offset");
static_assert(sizeof(S) == 10, "S size");
static_assert(alignof(S) == 1, "S ABI alignment");

struct alignas(4) ExplicitAlignas {
  char value;
};

struct __attribute__((aligned(4))) ExplicitAttribute {
  char value;
};

static_assert(alignof(ExplicitAlignas) == 4, "alignas must be honored");
static_assert(sizeof(ExplicitAlignas) == 4, "alignas affects size");
static_assert(alignof(ExplicitAttribute) == 4,
              "aligned attribute must be honored");
static_assert(sizeof(ExplicitAttribute) == 4,
              "aligned attribute affects size");

int use_layouts = sizeof(S) + sizeof(ExplicitAlignas) +
                  sizeof(ExplicitAttribute);

// CHECK:      0 | struct S
// CHECK-NEXT: 0 |   char a
// CHECK-NEXT: 1 |   long long b
// CHECK-NEXT: 9 |   char c
// CHECK-NEXT:   | [sizeof=10, dsize=10, align=1,

// CHECK:      0 | struct ExplicitAlignas
// CHECK-NEXT: 0 |   char value
// CHECK-NEXT:   | [sizeof=4, dsize=4, align=4,

// CHECK:      0 | struct ExplicitAttribute
// CHECK-NEXT: 0 |   char value
// CHECK-NEXT:   | [sizeof=4, dsize=4, align=4,
