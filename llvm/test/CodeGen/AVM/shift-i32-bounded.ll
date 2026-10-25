; RUN: llc -mtriple=avm -O0 -verify-machineinstrs %s -o - | FileCheck %s
; RUN: llc -mtriple=avm -O2 -verify-machineinstrs %s -o - | FileCheck %s
; RUN: llc -mtriple=avm -O2 -verify-machineinstrs -filetype=obj %s -o %t.o

; Widening shifts use native word shifts when both ranges are proven. Count
; zero must produce a zero carry, without relying on a masked shift by 16.
define i32 @word_shift7(i16 %input, i16 %count) {
; CHECK-LABEL: word_shift7:
; CHECK-NOT: call
; CHECK-DAG: lsr16.1
; CHECK-DAG: lsr16v
; CHECK-DAG: shl16v
; CHECK-NOT: call
; CHECK: ret
  %value = zext i16 %input to i32
  %bounded = and i16 %count, 7
  %widecount = zext i16 %bounded to i32
  %result = shl i32 %value, %widecount
  ret i32 %result
}

define i32 @word_shift15(i16 %input, i32 %count) {
; CHECK-LABEL: word_shift15:
; CHECK-NOT: call
; CHECK-DAG: lsr16.1
; CHECK-DAG: lsr16v
; CHECK-DAG: shl16v
; CHECK-NOT: call
; CHECK: ret
  %value = zext i16 %input to i32
  %bounded = and i32 %count, 15
  %result = shl i32 %value, %bounded
  ret i32 %result
}

; A nonzero range needs only one shift for the carry.
define i32 @word_shift_nonzero(i16 %input, i16 %count) {
; CHECK-LABEL: word_shift_nonzero:
; CHECK-NOT: call
; CHECK-NOT: lsr16.1
; CHECK-DAG: lsr16v
; CHECK-DAG: shl16v
; CHECK-NOT: call
; CHECK: ret
  %value = zext i16 %input to i32
  %masked = and i16 %count, 15
  %bounded = or i16 %masked, 1
  %widecount = zext i16 %bounded to i32
  %result = shl i32 %value, %widecount
  ret i32 %result
}

; Range information need not come from an explicit zero extension.
define i32 @masked_word_shift(i32 %input, i32 %count) {
; CHECK-LABEL: masked_word_shift:
; CHECK-NOT: call
; CHECK-DAG: lsr16v
; CHECK-DAG: shl16v
; CHECK-NOT: call
; CHECK: ret
  %value = and i32 %input, 65535
  %bounded = and i32 %count, 7
  %result = shl i32 %value, %bounded
  ret i32 %result
}

; Narrower inputs can eliminate the carry entirely.
define i32 @byte_shift7(i8 %input, i16 %count) {
; CHECK-LABEL: byte_shift7:
; CHECK-NOT: call
; CHECK: shl16v
; CHECK-NOT: call
; CHECK: ret
  %value = zext i8 %input to i32
  %masked = and i16 %count, 7
  %widecount = zext i16 %masked to i32
  %result = shl i32 %value, %widecount
  ret i32 %result
}

; The proof also applies to ranges derived without a low-bit mask.
define i32 @word_shift_derived(i16 %input, i16 %count) {
; CHECK-LABEL: word_shift_derived:
; CHECK-NOT: call
; CHECK-DAG: lsr16v
; CHECK-DAG: shl16v
; CHECK-NOT: call
; CHECK: ret
  %value = zext i16 %input to i32
  %bounded = lshr i16 %count, 12
  %widecount = zext i16 %bounded to i32
  %result = shl i32 %value, %widecount
  ret i32 %result
}

; Unproven input bits or counts retain the general helper.
define i32 @full_word_shift(i32 %input, i32 %count) {
; CHECK-LABEL: full_word_shift:
; CHECK: {{call|jmp}} __avm_ashlsi3
  %bounded = and i32 %count, 7
  %result = shl i32 %input, %bounded
  ret i32 %result
}

define i32 @signed_word_shift(i16 %input, i32 %count) {
; CHECK-LABEL: signed_word_shift:
; CHECK: {{call|jmp}} __avm_ashlsi3
  %value = sext i16 %input to i32
  %bounded = and i32 %count, 7
  %result = shl i32 %value, %bounded
  ret i32 %result
}

define i32 @word_shift31(i16 %input, i32 %count) {
; CHECK-LABEL: word_shift31:
; CHECK: {{call|jmp}} __avm_ashlsi3
  %value = zext i16 %input to i32
  %bounded = and i32 %count, 31
  %result = shl i32 %value, %bounded
  ret i32 %result
}

define i32 @word_shift16(i16 %input, i32 %count) {
; CHECK-LABEL: word_shift16:
; CHECK: {{call|jmp}} __avm_ashlsi3
  %value = zext i16 %input to i32
  %bounded = and i32 %count, 16
  %result = shl i32 %value, %bounded
  ret i32 %result
}

define i32 @wide_count(i16 %input, i32 %count) {
; CHECK-LABEL: wide_count:
; CHECK: {{call|jmp}} __avm_ashlsi3
  %value = zext i16 %input to i32
  %bounded = and i32 %count, 65543
  %result = shl i32 %value, %bounded
  ret i32 %result
}
