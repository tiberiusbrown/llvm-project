// RUN: %clang_cc1 -triple avm -emit-llvm -o - %s | FileCheck %s

#define AS1 __attribute__((address_space(1)))

void *constant_size(void *dst, const void AS1 *src) {
  return __builtin_avm_memcpy_p(dst, src, 7);
}

void *dynamic_size(void *dst, const void AS1 *src, unsigned short size) {
  return __builtin_avm_memcpy_p(dst, src, size);
}

extern void *next_dst(void);
extern const void AS1 *next_src(void);
extern unsigned short next_size(void);

void *side_effects(void) {
  return __builtin_avm_memcpy_p(next_dst(), next_src(), next_size());
}

// CHECK-LABEL: define{{.*}} ptr @constant_size(ptr noundef %dst, ptr addrspace(1) noundef %src)
// CHECK: [[DST:%.*]] = load ptr, ptr %dst.addr, align 1
// CHECK: [[SRC:%.*]] = load ptr addrspace(1), ptr %src.addr, align 1
// CHECK: call addrspace(1) void @llvm.memcpy.p0.p1.i16(ptr align 1 [[DST]], ptr addrspace(1) align 1 [[SRC]], i16 7, i1 false)
// CHECK: ret ptr [[DST]]
// CHECK-LABEL: define{{.*}} ptr @dynamic_size(ptr noundef %dst, ptr addrspace(1) noundef %src, i16 noundef zeroext %size)
// CHECK: [[DDST:%.*]] = load ptr, ptr %dst.addr, align 1
// CHECK: [[DSRC:%.*]] = load ptr addrspace(1), ptr %src.addr, align 1
// CHECK: [[SIZE:%.*]] = load i16, ptr %size.addr, align 1
// CHECK: call addrspace(1) void @llvm.memcpy.p0.p1.i16(ptr align 1 [[DDST]], ptr addrspace(1) align 1 [[DSRC]], i16 [[SIZE]], i1 false)
// CHECK: ret ptr [[DDST]]
// CHECK-LABEL: define{{.*}} ptr @side_effects()
// CHECK-COUNT-1: call{{.*}} ptr @next_dst()
// CHECK-COUNT-1: call{{.*}} ptr addrspace(1) @next_src()
// CHECK-COUNT-1: call{{.*}} i16 @next_size()
// CHECK: call addrspace(1) void @llvm.memcpy.p0.p1.i16
// CHECK-NOT: llvm.avm.memcpy
// CHECK-NOT: addrspacecast
// CHECK-NOT: ptrtoint
// CHECK-NOT: inttoptr
