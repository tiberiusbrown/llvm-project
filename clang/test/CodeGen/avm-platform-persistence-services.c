// RUN: %clang_cc1 -triple avm -emit-llvm -O0 -o - %s | FileCheck %s
// RUN: %clang_cc1 -triple avm -O0 -S -o - %s | FileCheck %s --check-prefix=ASM
// RUN: %clang_cc1 -triple avm -O2 -S -o - %s | FileCheck %s --check-prefix=ASM

extern unsigned char __avm_buttons(void);
extern void __avm_idle(void);
extern unsigned int __avm_generate_random_seed(void);
extern void __avm_save(void);
extern _Bool __avm_load(void);
extern _Bool __avm_save_exists(void);

unsigned char platform_buttons(void) {
  return __avm_buttons();
}

void platform_idle(void) {
  __avm_idle();
}

unsigned int platform_random_seed(void) {
  return __avm_generate_random_seed();
}

void persistence_save(void) {
  __avm_save();
}

_Bool persistence_load(void) {
  return __avm_load();
}

_Bool persistence_save_exists(void) {
  return __avm_save_exists();
}

unsigned int repeated_buttons(void) {
  return (unsigned int)__avm_buttons() + (unsigned int)__avm_buttons();
}

void persistence_sequence(void) {
  __avm_save();
  (void)__avm_load();
  (void)__avm_save_exists();
}

// CHECK-LABEL: define{{.*}} i8 @platform_buttons
// CHECK: call i8 @llvm.avm.buttons()
// CHECK-LABEL: define{{.*}} void @platform_idle
// CHECK: call void @llvm.avm.idle()
// CHECK-LABEL: define{{.*}} i16 @platform_random_seed
// CHECK: call i16 @llvm.avm.generate.random.seed()
// CHECK-LABEL: define{{.*}} void @persistence_save
// CHECK: call void @llvm.avm.save()
// CHECK-LABEL: define{{.*}} i1 @persistence_load
// CHECK: call i1 @llvm.avm.load()
// CHECK-LABEL: define{{.*}} i1 @persistence_save_exists
// CHECK: call i1 @llvm.avm.save.exists()
// CHECK-LABEL: define{{.*}} i16 @repeated_buttons
// CHECK: call i8 @llvm.avm.buttons()
// CHECK: call i8 @llvm.avm.buttons()
// CHECK-LABEL: define{{.*}} void @persistence_sequence
// CHECK: call void @llvm.avm.save()
// CHECK-NEXT: call i1 @llvm.avm.load()
// CHECK-NEXT: call i1 @llvm.avm.save.exists()

// ASM-LABEL: platform_buttons:
// ASM: sys buttons
// ASM-LABEL: platform_idle:
// ASM: sys idle
// ASM-LABEL: platform_random_seed:
// ASM: sys generate_random_seed
// ASM-LABEL: persistence_save:
// ASM: sys save
// ASM-LABEL: persistence_load:
// ASM: sys load
// ASM-LABEL: persistence_save_exists:
// ASM: sys save_exists
// ASM-LABEL: repeated_buttons:
// ASM: sys buttons
// ASM: sys buttons
// ASM-LABEL: persistence_sequence:
// ASM: sys save
// ASM: sys load
// ASM: sys save_exists
