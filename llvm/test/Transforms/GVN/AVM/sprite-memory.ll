; RUN: opt -passes=gvn -S %s | FileCheck %s

target triple = "avm-unknown-arduboyfx"

@offx = global i16 3, align 1
@__avm_framebuffer = external global [1024 x i8], align 1
@sprite = external addrspace(1) global [8 x i8], align 1

declare void @llvm.avm.draw.sprite.overwrite(
    i16, i16, ptr addrspace(1), i16, ptr)

define i16 @reuse_unrelated_global() {
; CHECK-LABEL: define i16 @reuse_unrelated_global(
; CHECK:         [[OFFX:%.*]] = load i16, ptr @offx
; CHECK-NEXT:    call{{.*}} void @llvm.avm.draw.sprite.overwrite
; CHECK-NOT:     load i16, ptr @offx
; CHECK:         [[SUM:%.*]] = add i16 [[OFFX]], [[OFFX]]
; CHECK:         ret i16 [[SUM]]
  %before = load i16, ptr @offx, align 1
  call void @llvm.avm.draw.sprite.overwrite(
      i16 1, i16 2, ptr addrspace(1) @sprite, i16 0,
      ptr @__avm_framebuffer)
  %after = load i16, ptr @offx, align 1
  %sum = add i16 %before, %after
  ret i16 %sum
}

define i8 @framebuffer_ordering() {
; CHECK-LABEL: define i8 @framebuffer_ordering(
; CHECK:         store i8 7, ptr @__avm_framebuffer
; CHECK-NEXT:    call{{.*}} void @llvm.avm.draw.sprite.overwrite
; CHECK-NEXT:    [[PIXEL:%.*]] = load i8, ptr @__avm_framebuffer
; CHECK:         ret i8 [[PIXEL]]
  store i8 7, ptr @__avm_framebuffer, align 1
  call void @llvm.avm.draw.sprite.overwrite(
      i16 1, i16 2, ptr addrspace(1) @sprite, i16 0,
      ptr @__avm_framebuffer)
  %pixel = load i8, ptr @__avm_framebuffer, align 1
  ret i8 %pixel
}
