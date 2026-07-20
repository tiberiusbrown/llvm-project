// RUN: %clang_cc1 -triple avm -emit-llvm -O2 -o - %s | FileCheck %s
// RUN: %clang_cc1 -triple avm -O0 -S -o - %s | FileCheck %s --check-prefix=ASM
// RUN: %clang_cc1 -triple avm -O2 -S -o - %s | FileCheck %s --check-prefix=ASM
// RUN: %clang_cc1 -triple avm -Os -S -o - %s | FileCheck %s --check-prefix=ASM
// RUN: %clang_cc1 -triple avm -Oz -S -o - %s | FileCheck %s --check-prefix=ASM

extern unsigned int __avm_millis(void);
extern unsigned long __avm_millis32(void);

void debug_services(unsigned char value) {
  __avm_debug_putc(value);
  __avm_debug_break();
}

unsigned long timer_services(void) {
  return (unsigned long)__avm_millis() + __avm_millis32();
}

float math_services(float x, float y) {
  float value = __avm_sinf(x);
  value = __avm_cosf(value);
  value = __avm_atan2f(value, y);
  value = __avm_tanf(value);
  value = __avm_expf(value);
  value = __avm_logf(value);
  value = __avm_log2f(value);
  value = __avm_log10f(value);
  value = __avm_powf(value, y);
  value = __avm_hypotf(value, x);
  return __avm_fmodf(value, y);
}

void *copy_service(void *dst, const void *src, unsigned short size) {
  return __avm_memcpy(dst, src, size);
}

void *fill_service(void *dst, int value, unsigned short size) {
  return __avm_memset(dst, value, size);
}

void *move_service(void *dst, const void *src, unsigned short size) {
  return __avm_memmove(dst, src, size);
}

float pressure(float x, float y) {
  return __avm_sinf(x) + __avm_powf(x, y);
}

// CHECK-LABEL: define{{.*}} void @debug_services
// CHECK: call{{.*}} void @llvm.avm.debug.putc(i8
// CHECK: call{{.*}} void @llvm.avm.debug.break()
// CHECK-LABEL: define{{.*}} i32 @timer_services
// CHECK: call{{.*}} i16 @llvm.avm.millis()
// CHECK: call{{.*}} i32 @llvm.avm.millis32()
// CHECK-LABEL: define{{.*}} float @math_services
// CHECK: call{{.*}} float @llvm.avm.sinf
// CHECK: call{{.*}} float @llvm.avm.cosf
// CHECK: call{{.*}} float @llvm.avm.atan2f
// CHECK: call{{.*}} float @llvm.avm.tanf
// CHECK: call{{.*}} float @llvm.avm.expf
// CHECK: call{{.*}} float @llvm.avm.logf
// CHECK: call{{.*}} float @llvm.avm.log2f
// CHECK: call{{.*}} float @llvm.avm.log10f
// CHECK: call{{.*}} float @llvm.avm.powf
// CHECK: call{{.*}} float @llvm.avm.hypotf
// CHECK: call{{.*}} float @llvm.avm.fmodf
// CHECK-LABEL: define{{.*}} ptr @copy_service
// CHECK: call{{.*}} ptr @llvm.avm.memcpy
// CHECK-LABEL: define{{.*}} ptr @fill_service
// CHECK: call{{.*}} ptr @llvm.avm.memset
// CHECK-LABEL: define{{.*}} ptr @move_service
// CHECK: call{{.*}} ptr @llvm.avm.memmove
// ASM-LABEL: pressure:
// ASM-DAG: sys sinf
// ASM-DAG: sys powf
