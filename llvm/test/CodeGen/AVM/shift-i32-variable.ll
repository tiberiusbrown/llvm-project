; RUN: llc -mtriple=avm -O0 -verify-machineinstrs %s -o - | FileCheck %s
; RUN: llc -mtriple=avm -O2 -verify-machineinstrs %s -o - | FileCheck %s
; RUN: llc -mtriple=avm -O2 -verify-machineinstrs -filetype=obj %s -o %t.o

; All native word counts must stay in 0..15, including a zero carry when the
; original count is zero. Exercise both directions and signed high words.
define i32 @logical_small(i32 %x, i32 %count) {
; CHECK-LABEL: logical_small:
; CHECK-NOT: __avm_
; CHECK-DAG: lsr16v
; CHECK-DAG: shl16v
; CHECK-NOT: __avm_
; CHECK: ret
  %n = and i32 %count, 15
  %r = lshr i32 %x, %n
  ret i32 %r
}

define i32 @arithmetic_small(i32 %x, i32 %count) {
; CHECK-LABEL: arithmetic_small:
; CHECK-NOT: __avm_
; CHECK-DAG: asr16v
; CHECK-DAG: lsr16v
; CHECK-DAG: shl16v
; CHECK-NOT: __avm_
; CHECK: ret
  %n = and i32 %count, 15
  %r = ashr i32 %x, %n
  ret i32 %r
}

define i32 @logical_word(i16 %x, i16 %count) {
; CHECK-LABEL: logical_word:
; CHECK-NOT: __avm_
; CHECK: lsr16v
; CHECK-NOT: shl16v
; CHECK-NOT: __avm_
; CHECK: ret
  %v = zext i16 %x to i32
  %small = and i16 %count, 15
  %n = zext i16 %small to i32
  %r = lshr i32 %v, %n
  ret i32 %r
}

define i32 @arithmetic_word(i16 %x, i16 %count) {
; CHECK-LABEL: arithmetic_word:
; CHECK-NOT: __avm_
; CHECK: asr16v
; CHECK-NOT: __avm_
; CHECK: ret
  %v = sext i16 %x to i32
  %small = and i16 %count, 15
  %n = zext i16 %small to i32
  %r = ashr i32 %v, %n
  ret i32 %r
}

define i32 @arithmetic_word_full(i16 %x, i16 %count) {
; CHECK-LABEL: arithmetic_word_full:
; CHECK-NOT: __avm_
; CHECK-DAG: asr16v
; CHECK-DAG: cmov.ne
; CHECK-NOT: shl16v
; CHECK-NOT: lsr16v
; CHECK-NOT: __avm_
; CHECK: ret
  %v = sext i16 %x to i32
  %small = and i16 %count, 31
  %n = zext i16 %small to i32
  %r = ashr i32 %v, %n
  ret i32 %r
}

; A nonzero small count does not need the extra one-bit carry shift.
define i32 @logical_nonzero(i32 %x, i16 %count) {
; CHECK-LABEL: logical_nonzero:
; CHECK-NOT: __avm_
; CHECK-DAG: lsr16v
; CHECK-DAG: shl16v
; CHECK-NOT: __avm_
; CHECK: ret
  %masked = and i16 %count, 15
  %nonzero = or i16 %masked, 1
  %n = zext i16 %nonzero to i32
  %r = lshr i32 %x, %n
  ret i32 %r
}

; A known count in 16..31 needs just one native variable shift, plus a sign
; fill for arithmetic right shift. No carry or conditional selection is needed.
define i32 @left_large(i32 %x, i32 %count) {
; CHECK-LABEL: left_large:
; CHECK-NOT: __avm_
; CHECK: shl16v
; CHECK-NOT: lsr16v
; CHECK-NOT: cmov
; CHECK-NOT: __avm_
; CHECK: ret
  %masked = and i32 %count, 15
  %n = or i32 %masked, 16
  %r = shl i32 %x, %n
  ret i32 %r
}

define i32 @logical_large(i32 %x, i32 %count) {
; CHECK-LABEL: logical_large:
; CHECK-NOT: __avm_
; CHECK: lsr16v
; CHECK-NOT: shl16v
; CHECK-NOT: cmov
; CHECK-NOT: __avm_
; CHECK: ret
  %masked = and i32 %count, 15
  %n = or i32 %masked, 16
  %r = lshr i32 %x, %n
  ret i32 %r
}

define i32 @arithmetic_large(i32 %x, i32 %count) {
; CHECK-LABEL: arithmetic_large:
; CHECK-NOT: __avm_
; CHECK: asr16v
; CHECK-NOT: shl16v
; CHECK-NOT: cmov
; CHECK-NOT: __avm_
; CHECK: ret
  %masked = and i32 %count, 15
  %n = or i32 %masked, 16
  %r = ashr i32 %x, %n
  ret i32 %r
}

; A count spanning both words selects the appropriate halves, sharing one
; native shift between the small-count and large-count results.
define i32 @left_full(i32 %x, i32 %count) {
; CHECK-LABEL: left_full:
; CHECK-NOT: __avm_
; CHECK-DAG: shl16v
; CHECK-DAG: lsr16v
; CHECK-DAG: cmov.ne
; CHECK-NOT: __avm_
; CHECK: ret
  %n = and i32 %count, 31
  %r = shl i32 %x, %n
  ret i32 %r
}

define i32 @logical_full(i32 %x, i32 %count) {
; CHECK-LABEL: logical_full:
; CHECK-NOT: __avm_
; CHECK-DAG: shl16v
; CHECK-DAG: lsr16v
; CHECK-DAG: cmov.ne
; CHECK-NOT: __avm_
; CHECK: ret
  %n = and i32 %count, 31
  %r = lshr i32 %x, %n
  ret i32 %r
}

define i32 @arithmetic_full(i32 %x, i32 %count) {
; CHECK-LABEL: arithmetic_full:
; CHECK-NOT: __avm_
; CHECK-DAG: shl16v
; CHECK-DAG: lsr16v
; CHECK-DAG: asr16v
; CHECK-DAG: cmov.ne
; CHECK-NOT: __avm_
; CHECK: ret
  %n = and i32 %count, 31
  %r = ashr i32 %x, %n
  ret i32 %r
}

; Unbounded counts retain the helper, including counts with unknown high bits.
define i32 @left_unbounded(i32 %x, i32 %n) {
; CHECK-LABEL: left_unbounded:
; CHECK: {{call|jmp}} __avm_ashlsi3
  %r = shl i32 %x, %n
  ret i32 %r
}

define i32 @logical_unbounded(i32 %x, i32 %n) {
; CHECK-LABEL: logical_unbounded:
; CHECK: {{call|jmp}} __avm_lshrsi3
  %r = lshr i32 %x, %n
  ret i32 %r
}

define i32 @arithmetic_unbounded(i32 %x, i32 %n) {
; CHECK-LABEL: arithmetic_unbounded:
; CHECK: {{call|jmp}} __avm_ashrsi3
  %r = ashr i32 %x, %n
  ret i32 %r
}
