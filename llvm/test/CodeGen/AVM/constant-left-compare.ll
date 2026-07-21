; RUN: llc -mtriple=avm -O2 -verify-machineinstrs < %s | FileCheck %s

declare void @true_path()
declare void @false_path()

define void @eq_branch(i16 %x) {
; CHECK-LABEL: eq_branch:
; CHECK: cmpi.s8 {{r[0-7]}}, 5
; CHECK: brne
  %c = icmp eq i16 5, %x
  br i1 %c, label %yes, label %no
yes:
  call void @true_path()
  ret void
no:
  call void @false_path()
  ret void
}

define i16 @ne_setcc(i16 %x) {
; CHECK-LABEL: ne_setcc:
; CHECK: cmpi.s8 {{r[0-7]}}, 7
; CHECK: cset.ne
  %c = icmp ne i16 7, %x
  %r = zext i1 %c to i16
  ret i16 %r
}

define i16 @ult_select(i16 %x, i16 %a, i16 %b) {
; CHECK-LABEL: ult_select:
; CHECK: cmpi.s8 {{r[0-7]}}, 2
; CHECK: cmov.uge
  %c = icmp ult i16 1, %x
  %r = select i1 %c, i16 %a, i16 %b
  ret i16 %r
}

define void @uge_branch(i16 %x) {
; CHECK-LABEL: uge_branch:
; CHECK: cmpi.s8 {{r[0-7]}}, 4
; CHECK: bruge
  %c = icmp uge i16 3, %x
  br i1 %c, label %yes, label %no
yes:
  call void @true_path()
  ret void
no:
  call void @false_path()
  ret void
}

define i16 @slt_setcc(i16 %x) {
; CHECK-LABEL: slt_setcc:
; CHECK: cmpi.s8 {{r[0-7]}}, 4
; CHECK: cset.sge
  %c = icmp slt i16 3, %x
  %r = zext i1 %c to i16
  ret i16 %r
}

define i16 @sge_select(i16 %x, i16 %a, i16 %b) {
; CHECK-LABEL: sge_select:
; CHECK: cmpi.s8 {{r[0-7]}}, 2
; CHECK: cmov.slt
  %c = icmp sge i16 1, %x
  %r = select i1 %c, i16 %a, i16 %b
  ret i16 %r
}

define i16 @eq_out_of_range(i16 %x) {
; CHECK-LABEL: eq_out_of_range:
; CHECK-NOT: cmpi.s8
; CHECK: ldi8 {{r[0-7]}}, 200
; CHECK: cmp
  %c = icmp eq i16 200, %x
  %r = zext i1 %c to i16
  ret i16 %r
}
