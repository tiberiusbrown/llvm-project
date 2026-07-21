; RUN: llc -mtriple=avm-unknown-arduboyfx -O2 -verify-machineinstrs %s -o %t.s
; RUN: llc -mtriple=avm-unknown-arduboyfx -O2 -verify-machineinstrs -filetype=obj %s -o %t.o

target triple = "avm-unknown-arduboyfx"

declare i16 @llvm.fshl.i16(i16, i16, i16)
declare i16 @llvm.fshr.i16(i16, i16, i16)
declare i32 @llvm.fshl.i32(i32, i32, i32)
declare i32 @llvm.fshr.i32(i32, i32, i32)

define i16 @rotl16(i16 %value, i16 %count) {
entry:
  %result = call i16 @llvm.fshl.i16(
      i16 %value, i16 %value, i16 %count)
  ret i16 %result
}

define i16 @rotr16(i16 %value, i16 %count) {
entry:
  %result = call i16 @llvm.fshr.i16(
      i16 %value, i16 %value, i16 %count)
  ret i16 %result
}

define i32 @rotl32(i32 %value, i32 %count) {
entry:
  %result = call i32 @llvm.fshl.i32(
      i32 %value, i32 %value, i32 %count)
  ret i32 %result
}

define i32 @rotr32(i32 %value, i32 %count) {
entry:
  %result = call i32 @llvm.fshr.i32(
      i32 %value, i32 %value, i32 %count)
  ret i32 %result
}
