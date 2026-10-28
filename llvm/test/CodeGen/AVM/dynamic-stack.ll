; RUN: llc -mtriple=avm -O0 -verify-machineinstrs < %s | FileCheck %s
; RUN: llc -mtriple=avm -O2 -verify-machineinstrs < %s | FileCheck %s

declare ptr @llvm.stacksave.p0()
declare void @llvm.stackrestore.p0(ptr)
declare void @use(ptr, i16, i16, i16, i16, i16, i16, i16)

; Fixed objects must survive both dynamic allocations and call argument areas.
define i16 @scopes(i16 %n, i16 %x) {
; CHECK-LABEL: scopes:
; CHECK: push16 r3
; CHECK: getsp r3
; CHECK: getsp
; CHECK: sub
; CHECK: setsp
; CHECK: getsp
; CHECK: sub
; CHECK: setsp
; CHECK: adjsp -
; CHECK: call use
; CHECK: adjsp {{[0-9]+}}
; CHECK: setsp
; CHECK: ld16
; CHECK: setsp r3
; CHECK: pop16 r3
; CHECK: ret
  %fixed = alloca i16, align 1
  store volatile i16 %x, ptr %fixed, align 1
  %save = call ptr @llvm.stacksave.p0()
  %a = alloca i8, i16 %n, align 1
  store volatile i8 17, ptr %a, align 1
  %inner = call ptr @llvm.stacksave.p0()
  %b = alloca i32, i16 %n, align 1
  store volatile i32 123456, ptr %b, align 1
  call void @use(ptr %b, i16 1, i16 2, i16 3, i16 4, i16 5, i16 6, i16 7)
  call void @llvm.stackrestore.p0(ptr %inner)
  %v = load volatile i8, ptr %a, align 1
  call void @use(ptr %a, i16 1, i16 2, i16 3, i16 4, i16 5, i16 6, i16 7)
  call void @llvm.stackrestore.p0(ptr %save)
  %result = load volatile i16, ptr %fixed, align 1
  ret i16 %result
}

; CHECK-LABEL: ordinary:
; CHECK-NOT: push16 r3
; CHECK-NOT: getsp r3
; CHECK: ret
define i16 @ordinary(i16 %n) {
  %v = add i16 %n, 1
  ret i16 %v
}

declare void @tail_target()

; A tail transfer releases the active dynamic area before restoring r3.
define void @dynamic_tail(i16 %n) {
; CHECK-LABEL: dynamic_tail:
; CHECK: getsp r3
; CHECK: setsp
; CHECK: setsp r3
; CHECK: pop16 r3
; CHECK: jmp tail_target
  %a = alloca i8, i16 %n, align 1
  store volatile i8 42, ptr %a
  tail call void @tail_target()
  ret void
}
