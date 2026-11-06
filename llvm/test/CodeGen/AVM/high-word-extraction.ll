; RUN: llc -mtriple=avm -O2 -stop-after=finalize-isel -verify-machineinstrs %s -o - | FileCheck %s
; RUN: llc -mtriple=avm -O2 -verify-machineinstrs %s -o /dev/null

; Extract the input word directly. A temporary pair for the right shift can
; constrain a scalar reduction to even registers after register coalescing.
define i16 @logical_high_word(ptr addrspace(1) %source) {
; CHECK-LABEL: name: logical_high_word
; CHECK-NOT: SRL32_16_PSEUDO
; CHECK: COPY %{{[0-9]+}}.sub_hi16
; CHECK-NOT: SRL32_16_PSEUDO
; CHECK: RET_PSEUDO
  %value = load volatile i32, ptr addrspace(1) %source, align 1
  %shifted = lshr i32 %value, 16
  %result = trunc i32 %shifted to i16
  ret i16 %result
}

define i16 @arithmetic_high_word(ptr addrspace(1) %source) {
; CHECK-LABEL: name: arithmetic_high_word
; CHECK-NOT: SRA32_16_PSEUDO
; CHECK: COPY %{{[0-9]+}}.sub_hi16
; CHECK-NOT: SRA32_16_PSEUDO
; CHECK: RET_PSEUDO
  %value = load volatile i32, ptr addrspace(1) %source, align 1
  %shifted = ashr i32 %value, 16
  %result = trunc i32 %shifted to i16
  ret i16 %result
}
