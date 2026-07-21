; RUN: llc -mtriple=avm -O2 -verify-machineinstrs < %s | FileCheck %s
; RUN: llc -mtriple=avm -O2 -verify-machineinstrs -filetype=obj %s -o %t.o

declare void @less_path()
declare void @other_path()

define float @select_olt(float %true, float %false, float %a, float %b) {
; CHECK-LABEL: select_olt:
; CHECK:       fcmp
; CHECK:       cmpi.s8 {{r[0-7]}}, {{-0x1|-1}}
; CHECK-NOT:   cset
; CHECK-NOT:   tst16
; CHECK:       cmov.eq
; CHECK:       cmov.eq
  %condition = fcmp olt float %a, %b
  %result = select i1 %condition, float %true, float %false
  ret float %result
}

define float @select_ogt(float %true, float %false, float %a, float %b) {
; CHECK-LABEL: select_ogt:
; CHECK:       fcmp
; CHECK:       cmpi.s8 {{r[0-7]}}, 1
; CHECK-NOT:   cset
; CHECK-NOT:   tst16
; CHECK:       cmov.eq
; CHECK:       cmov.eq
  %condition = fcmp ogt float %a, %b
  %result = select i1 %condition, float %true, float %false
  ret float %result
}

define void @branch_olt(float %a, float %b) {
; CHECK-LABEL: branch_olt:
; CHECK:       fcmp
; CHECK:       cmpi.s8 {{r[0-7]}}, {{-0x1|-1}}
; CHECK-NOT:   cset
; CHECK-NOT:   tst16
; CHECK:       brne
entry:
  %condition = fcmp olt float %a, %b
  br i1 %condition, label %less, label %other

less:
  call void @less_path()
  ret void

other:
  call void @other_path()
  ret void
}
