// RUN: %clang_cc1 -triple avm -emit-llvm -o - %s | FileCheck %s --check-prefixes=CHECK,C
// RUN: %clang_cc1 -triple avm -x c++ -std=c++20 -emit-llvm -o - %s | FileCheck %s

#define AS1 __attribute__((address_space(1)))
#ifdef __cplusplus
extern "C" {
#define CONSTANT constexpr
#define EXTERNAL extern
#else
#define CONSTANT const
#define EXTERNAL
#endif

struct LargeRecord {
  unsigned char first[32768UL];
  unsigned char second[32768UL];
  unsigned char last;
};
union LargeUnion {
  unsigned char bytes[65537UL];
  struct LargeRecord record;
};
EXTERNAL CONSTANT struct LargeRecord AS1 large_record = {{1}, {2}, 3};
EXTERNAL CONSTANT union LargeUnion AS1 large_union = {{6}};
EXTERNAL CONSTANT unsigned char AS1 large_array[65537UL] = {4};
EXTERNAL CONSTANT unsigned long long AS1 large_words[9818] = {5};
EXTERNAL const void AS1 *const record_pointer = &large_record;
EXTERNAL const void AS1 *const array_pointer = large_array;

unsigned char read_array(unsigned long index) { return large_array[index]; }
unsigned char read_record(const struct LargeRecord AS1 *p) { return p->last; }

// CHECK-DAG: %struct.LargeRecord = type { [32768 x i8], [32768 x i8], i8 }
// CHECK-DAG: @large_record ={{.*}} addrspace(1) constant {{.*}}[32767 x i8]{{.*}}i8 3 }, align 1
// CHECK-DAG: @large_union ={{.*}} addrspace(1) constant {{.*}}[65536 x i8]
// CHECK-DAG: @large_array ={{.*}} addrspace(1) constant <{ i8, [65536 x i8] }>
// CHECK-DAG: @large_words ={{.*}} addrspace(1) constant <{ i64, [9817 x i64] }>
// CHECK-DAG: @record_pointer ={{.*}} constant ptr addrspace(1) @large_record, align 1
// CHECK-DAG: @array_pointer ={{.*}} constant ptr addrspace(1) @large_array, align 1
// CHECK-LABEL: define{{.*}} i8 @read_array(
// CHECK: getelementptr inbounds nuw [65537 x i8], ptr addrspace(1) @large_array, i24 0, i24
// CHECK: load i8, ptr addrspace(1)
// CHECK-LABEL: define{{.*}} i8 @read_record(
// CHECK: getelementptr inbounds nuw %struct.LargeRecord, ptr addrspace(1) {{.*}}, i32 0, i32 2
// CHECK: load i8, ptr addrspace(1)

#ifndef __cplusplus
unsigned char read_vla(unsigned n, const unsigned char AS1 (*p)[n],
                       unsigned long row, unsigned column) {
  return p[row][column];
}
// C-LABEL: define{{.*}} i8 @read_vla(
// C: zext i16 {{.*}} to i24
// C: mul nsw i24
// C: load i8, ptr addrspace(1)
#endif

#ifdef __cplusplus
}
#endif
