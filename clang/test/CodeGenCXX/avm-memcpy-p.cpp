// RUN: %clang_cc1 -triple avm -std=c++17 -emit-llvm -o - %s | FileCheck %s

#define AS1 __attribute__((address_space(1)))
extern void *next_dst();
extern const void AS1 *next_src();
extern unsigned short next_size();

void *side_effects() {
  return __builtin_avm_memcpy_p(next_dst(), next_src(), next_size());
}

// CHECK-LABEL: define{{.*}} ptr @_Z12side_effectsv()
// CHECK-COUNT-1: call{{.*}} ptr @_Z8next_dstv()
// CHECK-COUNT-1: call{{.*}} ptr addrspace(1) @_Z8next_srcv()
// CHECK-COUNT-1: call{{.*}} i16 @_Z9next_sizev()
// CHECK: call addrspace(1) void @llvm.memcpy.p0.p1.i16
// CHECK-NOT: addrspacecast
// CHECK-NOT: ptrtoint
// CHECK-NOT: inttoptr
