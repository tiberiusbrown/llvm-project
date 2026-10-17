// RUN: %clang_cc1 -triple avm -emit-llvm -O0 -o - %s | FileCheck %s
// RUN: %clang_cc1 -triple avm -O2 -S -o - %s | FileCheck %s --check-prefix=ASM
// RUN: %clang_cc1 -triple avm -x c++ -emit-llvm -O0 -o - %s | FileCheck %s

#ifdef __cplusplus
extern "C" {
#endif

unsigned int width(void) { return __avm_get_system_width(); }
unsigned int height(void) { return __avm_get_system_height(); }
unsigned char display_type(void) { return __avm_get_system_display_type(); }
void commands(unsigned char const *cmd, __SIZE_TYPE__ n) {
  __avm_send_display_command(cmd, n);
}

#ifdef __cplusplus
}
#endif

// CHECK-LABEL: define{{.*}} i16 @width(
// CHECK: call{{.*}} i16 @llvm.avm.get.system.width()
// CHECK-LABEL: define{{.*}} i16 @height(
// CHECK: call{{.*}} i16 @llvm.avm.get.system.height()
// CHECK-LABEL: define{{.*}} i8 @display_type(
// CHECK: [[TYPE:%.*]] = call{{.*}} i16 @llvm.avm.get.system.display.type()
// CHECK: trunc i16 [[TYPE]] to i8
// CHECK-LABEL: define{{.*}} void @commands(
// CHECK: call{{.*}} void @llvm.avm.send.display.command(ptr {{.*}}, i16 {{.*}})
// CHECK: declare void @llvm.avm.send.display.command(ptr readonly captures(none), i16) addrspace(1) #[[IO:[0-9]+]]
// CHECK: attributes #[[IO]] = { {{.*}}memory(argmem: read, inaccessiblemem: readwrite) }

// ASM-LABEL: width:
// ASM: sys get_system_width
// ASM-LABEL: height:
// ASM: sys get_system_height
// ASM-LABEL: display_type:
// ASM: sys get_system_display_type
// ASM-LABEL: commands:
// ASM: sys send_display_command
