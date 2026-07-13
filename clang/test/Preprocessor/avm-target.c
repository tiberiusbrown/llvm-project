// RUN: %clang_cc1 -triple avm-unknown-arduboyfx -E -dM %s | FileCheck %s

// CHECK-DAG: #define AVM 1
// CHECK-DAG: #define __AVM 1
// CHECK-DAG: #define __AVM__ 1
// CHECK-DAG: #define __ARDUBOY__ 1
// CHECK-DAG: #define __ARDUBOY_FX__ 1
// CHECK-DAG: #define __BYTE_ORDER__ __ORDER_LITTLE_ENDIAN__
// CHECK-DAG: #define __CHAR_BIT__ 8
// CHECK-DAG: #define __SIZEOF_SHORT__ 2
// CHECK-DAG: #define __SIZEOF_INT__ 2
// CHECK-DAG: #define __SIZEOF_LONG__ 4
// CHECK-DAG: #define __SIZEOF_LONG_LONG__ 8
// CHECK-DAG: #define __SIZEOF_FLOAT__ 4
// CHECK-DAG: #define __SIZEOF_DOUBLE__ 4
// CHECK-DAG: #define __SIZEOF_LONG_DOUBLE__ 4
// CHECK-DAG: #define __SIZEOF_POINTER__ 2
// CHECK-DAG: #define __SIZEOF_SIZE_T__ 2
// CHECK-DAG: #define __SIZEOF_PTRDIFF_T__ 2
// CHECK-DAG: #define __SIZEOF_WCHAR_T__ 2
// CHECK-DAG: #define __SIZE_TYPE__ unsigned int
// CHECK-DAG: #define __PTRDIFF_TYPE__ int
// CHECK-DAG: #define __WCHAR_TYPE__ unsigned int
