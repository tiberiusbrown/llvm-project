; RUN: llc -mtriple=avm -O0 -verify-machineinstrs -filetype=obj %s -o %t.0
; RUN: llc -mtriple=avm -O1 -verify-machineinstrs -filetype=obj %s -o %t.1
; RUN: llc -mtriple=avm -O2 -verify-machineinstrs -filetype=obj %s -o %t.2
; RUN: llc -mtriple=avm -O3 -verify-machineinstrs -filetype=obj %s -o %t.3
; RUN: llc -mtriple=avm -O2 %s -o - | FileCheck %s --implicit-check-not=fcmp --implicit-check-not=cmp32

declare i1 @llvm.is.fpclass.f32(float, i32 immarg)

define i1 @empty(float %x) {
; CHECK-LABEL: empty:
  %p = call i1 @llvm.is.fpclass.f32(float %x, i32 0)
  ret i1 %p
}

define i1 @all(float %x) {
; CHECK-LABEL: all:
  %p = call i1 @llvm.is.fpclass.f32(float %x, i32 1023)
  ret i1 %p
}

define i1 @snan(float %x) {
; CHECK-LABEL: snan:
; CHECK: cset.
  %p = call i1 @llvm.is.fpclass.f32(float %x, i32 1)
  ret i1 %p
}

define i1 @qnan(float %x) {
; CHECK-LABEL: qnan:
; CHECK: cset.
  %p = call i1 @llvm.is.fpclass.f32(float %x, i32 2)
  ret i1 %p
}

define i1 @ninf(float %x) {
; CHECK-LABEL: ninf:
; CHECK: cset.
  %p = call i1 @llvm.is.fpclass.f32(float %x, i32 4)
  ret i1 %p
}

define i1 @nnormal(float %x) {
; CHECK-LABEL: nnormal:
; CHECK: cset.
  %p = call i1 @llvm.is.fpclass.f32(float %x, i32 8)
  ret i1 %p
}

define i1 @nsubnormal(float %x) {
; CHECK-LABEL: nsubnormal:
; CHECK: cset.
  %p = call i1 @llvm.is.fpclass.f32(float %x, i32 16)
  ret i1 %p
}

define i1 @nzero(float %x) {
; CHECK-LABEL: nzero:
; CHECK: cset.
  %p = call i1 @llvm.is.fpclass.f32(float %x, i32 32)
  ret i1 %p
}

define i1 @pzero(float %x) {
; CHECK-LABEL: pzero:
; CHECK: cset.
  %p = call i1 @llvm.is.fpclass.f32(float %x, i32 64)
  ret i1 %p
}

define i1 @psubnormal(float %x) {
; CHECK-LABEL: psubnormal:
; CHECK: cset.
  %p = call i1 @llvm.is.fpclass.f32(float %x, i32 128)
  ret i1 %p
}

define i1 @pnormal(float %x) {
; CHECK-LABEL: pnormal:
; CHECK: cset.
  %p = call i1 @llvm.is.fpclass.f32(float %x, i32 256)
  ret i1 %p
}

define i1 @pinf(float %x) {
; CHECK-LABEL: pinf:
; CHECK: cset.
  %p = call i1 @llvm.is.fpclass.f32(float %x, i32 512)
  ret i1 %p
}

define i1 @finite(float %x) {
; CHECK-LABEL: finite:
; CHECK: and{{.*}}r5
; CHECK: cset.
; CHECK-NEXT: ret
  %p = call i1 @llvm.is.fpclass.f32(float %x, i32 504)
  ret i1 %p
}

define i1 @normal(float %x) {
; CHECK-LABEL: normal:
; CHECK: and{{.*}}r5
; CHECK: cset.
; CHECK-NEXT: ret
  %p = call i1 @llvm.is.fpclass.f32(float %x, i32 264)
  ret i1 %p
}

define i1 @nan(float %x) {
; CHECK-LABEL: nan:
; CHECK: cset.
  %p = call i1 @llvm.is.fpclass.f32(float %x, i32 3)
  ret i1 %p
}

define i1 @inf(float %x) {
; CHECK-LABEL: inf:
; CHECK: cset.
  %p = call i1 @llvm.is.fpclass.f32(float %x, i32 516)
  ret i1 %p
}

define i1 @zero(float %x) {
; CHECK-LABEL: zero:
; CHECK: cset.
  %p = call i1 @llvm.is.fpclass.f32(float %x, i32 96)
  ret i1 %p
}

define i1 @subnormal(float %x) {
; CHECK-LABEL: subnormal:
; CHECK: cset.
  %p = call i1 @llvm.is.fpclass.f32(float %x, i32 144)
  ret i1 %p
}

define i1 @alternating(float %x) {
; CHECK-LABEL: alternating:
; CHECK: cset.
  %p = call i1 @llvm.is.fpclass.f32(float %x, i32 341)
  ret i1 %p
}

define i1 @alternating_inverse(float %x) {
; CHECK-LABEL: alternating_inverse:
; CHECK: cset.
  %p = call i1 @llvm.is.fpclass.f32(float %x, i32 682)
  ret i1 %p
}

define i1 @arbitrary(float %x) {
; CHECK-LABEL: arbitrary:
; CHECK: cset.
  %p = call i1 @llvm.is.fpclass.f32(float %x, i32 777)
  ret i1 %p
}


define i1 @high_word_infinity_guard(float %x) {
; CHECK-LABEL: high_word_infinity_guard:
; CHECK-NOT: cmp32
; CHECK-NOT: sub
; CHECK: and{{.*}}r5
; CHECK: cmp
; CHECK: cset.eq
; CHECK-NEXT: ret
  %bits = bitcast float %x to i32
  %high = and i32 %bits, 2147418112
  %p = icmp eq i32 %high, 2139095040
  ret i1 %p
}

declare void @nonfinite()
define void @finite_branch(float %x) {
; CHECK-LABEL: finite_branch:
; CHECK-NOT: cset
; CHECK-NOT: tst
; CHECK: cmp
; CHECK-NOT: cset
; CHECK-NOT: tst
; CHECK: br
  %p = call i1 @llvm.is.fpclass.f32(float %x, i32 504)
  br i1 %p, label %done, label %exception
exception:
  call void @nonfinite()
  br label %done
done:
  ret void
}

define void @high_word_sign_guard(float %x) {
; CHECK-LABEL: high_word_sign_guard:
; CHECK: tst16 r5
; CHECK-NEXT: br
  %bits = bitcast float %x to i32
  %p = icmp slt i32 %bits, 0
  br i1 %p, label %negative, label %done
negative:
  call void @nonfinite()
  br label %done
done:
  ret void
}

define i1 @high_word_upper_bound(float %x) {
; CHECK-LABEL: high_word_upper_bound:
; CHECK: ldi16 [[UPPER:r[0-7]]], 16201
; CHECK-NEXT: cmp r5, [[UPPER]]
; CHECK-NEXT: cset.ult r4
; CHECK-NEXT: ret
  %bits = bitcast float %x to i32
  %p = icmp ult i32 %bits, 1061748736
  ret i1 %p
}

define i1 @high_word_lower_bound(float %x) {
; CHECK-LABEL: high_word_lower_bound:
; CHECK: cmp{{.*}}r5
; CHECK-NEXT: cset.ult r4
; CHECK-NEXT: ret
  %bits = bitcast float %x to i32
  %p = icmp ugt i32 %bits, 1061748735
  ret i1 %p
}
