// RUN: %clang_cc1 -triple avm -std=c++23 -emit-llvm -o - %s | FileCheck %s
// RUN: %clang_cc1 -triple avm -std=c++23 -fwritable-strings -emit-llvm -o - %s \
// RUN:   | FileCheck %s --check-prefix=WRITABLE

// Flash placement follows GlobalVariable::isConstant. Immutable AS0 storage is
// constant. Mutable subobjects, dynamic initialization, and writable literals
// are not, and program-space objects keep their address space.

#define AS1 __attribute__((address_space(1)))

int get_value();

constexpr int values[] = {10, 20, 30};
struct Point {
  int x;
  int y;
};
constexpr Point origin{0, 0};
struct Counter {
  mutable int value;
};
const Counter counter{1};
const int runtime_value = get_value();
static const char *writable_pointer = "Hello";
const unsigned char AS1 program_bytes[] = {1, 2, 3, 4};

int use_locals() {
  static const int local_constant = 7;
  static const int local_dynamic = get_value();
  return local_constant + local_dynamic;
}

const int *values_at(int index) { return values + index; }
const Point *origin_address() { return &origin; }
const Counter *counter_address() { return &counter; }
const int *runtime_address() { return &runtime_value; }
const char *writable_text() { return writable_pointer; }
const unsigned char AS1 *program_address() { return program_bytes; }
int locals() { return use_locals(); }

// CHECK-DAG: @_ZL6values = internal constant [3 x i16] [i16 10, i16 20, i16 30]
// CHECK-DAG: @_ZL6origin = internal constant %struct.Point zeroinitializer
// CHECK-DAG: @_ZL7counter = internal global %struct.Counter { i16 1 }
// CHECK-DAG: @_ZL13runtime_value = internal global i16 0
// CHECK-DAG: @_ZZ10use_localsvE14local_constant = internal constant i16 7
// CHECK-DAG: @_ZZ10use_localsvE13local_dynamic = internal global i16 0
// CHECK-DAG: @_ZL16writable_pointer = internal global ptr @.str
// CHECK-DAG: @.str = private unnamed_addr constant [6 x i8] c"Hello\00"
// CHECK-DAG: @_ZL13program_bytes = internal addrspace(1) constant [4 x i8]

// WRITABLE-DAG: @.str = private unnamed_addr global [6 x i8] c"Hello\00"
// WRITABLE-NOT: @.str = private unnamed_addr constant
