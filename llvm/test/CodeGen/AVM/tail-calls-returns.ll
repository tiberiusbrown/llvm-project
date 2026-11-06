; RUN: llc -mtriple=avm -O0 -verify-machineinstrs %s -o - | FileCheck %s
; RUN: llc -mtriple=avm -O2 -verify-machineinstrs %s -o - | FileCheck %s

declare signext i8 @ret8()
declare i32 @ret32()
declare i64 @ret64()
declare float @retfloat()
declare ptr @retptr()
declare ptr addrspace(1) @retprogptr()

define signext i8 @tail8() nounwind {
; CHECK-LABEL: tail8:
; CHECK: adjsp 1
; CHECK-NEXT: jmp ret8
; CHECK-NOT: {{^ret([ \t]|$)}}
; CHECK: .Lfunc_end
  %buf = alloca i8, align 1
  store volatile i8 1, ptr %buf
  %r = tail call signext i8 @ret8()
  ret i8 %r
}
define i32 @tail32() nounwind {
; CHECK-LABEL: tail32:
; CHECK: adjsp 1
; CHECK-NEXT: jmp ret32
; CHECK-NOT: {{^ret([ \t]|$)}}
; CHECK: .Lfunc_end
  %buf = alloca i8, align 1
  store volatile i8 1, ptr %buf
  %r = tail call i32 @ret32()
  ret i32 %r
}
define i64 @tail64() nounwind {
; CHECK-LABEL: tail64:
; CHECK: adjsp 1
; CHECK-NEXT: jmp ret64
; CHECK-NOT: {{^ret([ \t]|$)}}
; CHECK: .Lfunc_end
  %buf = alloca i8, align 1
  store volatile i8 1, ptr %buf
  %r = tail call i64 @ret64()
  ret i64 %r
}
define float @tailfloat() nounwind {
; CHECK-LABEL: tailfloat:
; CHECK: adjsp 1
; CHECK-NEXT: jmp retfloat
; CHECK-NOT: {{^ret([ \t]|$)}}
; CHECK: .Lfunc_end
  %buf = alloca i8, align 1
  store volatile i8 1, ptr %buf
  %r = tail call float @retfloat()
  ret float %r
}
define ptr @tailptr() nounwind {
; CHECK-LABEL: tailptr:
; CHECK: adjsp 1
; CHECK-NEXT: jmp retptr
; CHECK-NOT: {{^ret([ \t]|$)}}
; CHECK: .Lfunc_end
  %buf = alloca i8, align 1
  store volatile i8 1, ptr %buf
  %r = tail call ptr @retptr()
  ret ptr %r
}
define ptr addrspace(1) @tailprogptr() nounwind {
; CHECK-LABEL: tailprogptr:
; CHECK: adjsp 1
; CHECK-NEXT: jmp retprogptr
; CHECK-NOT: {{^ret([ \t]|$)}}
; CHECK: .Lfunc_end
  %buf = alloca i8, align 1
  store volatile i8 1, ptr %buf
  %r = tail call ptr addrspace(1) @retprogptr()
  ret ptr addrspace(1) %r
}
