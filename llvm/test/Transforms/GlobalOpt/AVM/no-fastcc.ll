; RUN: opt -mtriple=avm-unknown-arduboyfx -passes=globalopt -S %s -o %t.ll
; RUN: FileCheck %s --implicit-check-not='{{[[:space:]]fastcc[[:space:]]}}' < %t.ll
; RUN: llc -mtriple=avm-unknown-arduboyfx -verify-machineinstrs %t.ll -o /dev/null

target triple = "avm-unknown-arduboyfx"

define i16 @caller(i16 %value) {
entry:
  %result = call i16 @helper(i16 %value)
  ret i16 %result
}

define internal i16 @helper(i16 %value) noinline {
entry:
  %result = add i16 %value, 1
  ret i16 %result
}

; CHECK-LABEL: define i16 @caller(
; CHECK: call{{.*}} i16 @helper(
; CHECK-LABEL: define internal i16 @helper(
