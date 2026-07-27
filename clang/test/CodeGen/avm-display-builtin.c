// RUN: %clang_cc1 -triple avm-unknown-arduboyfx -ffreestanding \
// RUN:   -emit-llvm -o - %s | FileCheck %s

void display_false(void) {
  // CHECK-LABEL: define{{.*}} void @display_false()
  // CHECK: call void @llvm.avm.display(i16 0)
  __avm_display(0);
}

void display_true(void) {
  // CHECK-LABEL: define{{.*}} void @display_true()
  // CHECK: call void @llvm.avm.display(i16 1)
  __avm_display(1);
}

void display_dynamic(_Bool clear) {
  // CHECK-LABEL: define{{.*}} void @display_dynamic(
  // CHECK: [[CLEAR:%.*]] = zext i1 %{{.*}} to i16
  // CHECK: call void @llvm.avm.display(i16 [[CLEAR]])
  __avm_display(clear);
}
