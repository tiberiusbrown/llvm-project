// RUN: %clang_cc1 -triple avm -emit-llvm -O0 -o - %s | FileCheck %s
// RUN: %clang_cc1 -triple avm -O0 -S -o - %s | FileCheck %s --check-prefix=ASM
// RUN: %clang_cc1 -triple avm -O2 -S -o - %s | FileCheck %s --check-prefix=ASM

void filled_rect_services(int x, int y, unsigned char width,
                          unsigned char height) {
  __avm_draw_filled_rect_white(x, y, width, height);
  __avm_draw_filled_rect_black(x, y, width, height);
}

// CHECK: module asm ".globl __avm_framebuffer"
// CHECK: @__avm_framebuffer = external global [1024 x i8], align 1
//
// CHECK-LABEL: define{{.*}} void @filled_rect_services
// CHECK: call{{.*}} void @llvm.avm.draw.filled.rect.white(
// CHECK-SAME: i16 {{.*}},
// CHECK-SAME: i16 {{.*}},
// CHECK-SAME: i8 {{.*}},
// CHECK-SAME: i8 {{.*}},
// CHECK-SAME: ptr @__avm_framebuffer)
// CHECK: call{{.*}} void @llvm.avm.draw.filled.rect.black(
// CHECK-SAME: i16 {{.*}},
// CHECK-SAME: i16 {{.*}},
// CHECK-SAME: i8 {{.*}},
// CHECK-SAME: i8 {{.*}},
// CHECK-SAME: ptr @__avm_framebuffer)
// CHECK: declare void @llvm.avm.draw.filled.rect.white(i16, i16, i8, i8, ptr
// CHECK: declare void @llvm.avm.draw.filled.rect.black(i16, i16, i8, i8, ptr
//
// ASM-LABEL: filled_rect_services:
// ASM: sys draw_filled_rect_white
// ASM: sys draw_filled_rect_black
