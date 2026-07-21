; RUN: llc -mtriple=avm-unknown-arduboyfx -O2 -verify-machineinstrs %s -o %t.s
; RUN: llc -mtriple=avm-unknown-arduboyfx -O2 -verify-machineinstrs -filetype=obj %s -o %t.o

target triple = "avm-unknown-arduboyfx"

define i32 @sext_i8_to_i32(i32 %value) {
entry:
  %byte = trunc i32 %value to i8
  %extended = sext i8 %byte to i32
  ret i32 %extended
}
