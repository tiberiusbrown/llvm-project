; RUN: llc -mtriple=avm-unknown-arduboyfx -verify-machineinstrs < %s | FileCheck %s

declare i16 @callee(i16)

define i16 @unsupported_call(i16 %value) {
  %result = call i16 @callee(i16 %value)
  ret i16 %result
}

; CHECK-LABEL: unsupported_call:
; CHECK: callf callee
; CHECK: ret

define i16 @five_args(i16 %a, i16 %b, i16 %c, i16 %d, i16 %e) {
  %x = add i16 %a, %e
  ret i16 %x
}

; CHECK-LABEL: five_args:
; CHECK: ldsp16
; CHECK-SAME: [sp+3]
; CHECK: ret

define i16 @call_five(i16 %a) {
  %result = call i16 @five_args(i16 %a, i16 2, i16 3, i16 4, i16 5)
  ret i16 %result
}

; CHECK-LABEL: call_five:
; CHECK: adjsp -2
; CHECK: st16
; CHECK: callf five_args
; CHECK: adjsp 2
; CHECK: ret
