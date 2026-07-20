; RUN: llc -mtriple=avm-unknown-arduboyfx -mcpu=avm1 \
; RUN:   -mtune=avm-interpreter-32u4-v1 -O0 -verify-machineinstrs < %s \
; RUN:   | FileCheck %s

define i8 @atomic_load8(ptr %p) {
; CHECK-LABEL: atomic_load8:
; CHECK:       ld8u
; CHECK-NOT:   __atomic
; CHECK:       ret
  %value = load atomic i8, ptr %p seq_cst, align 1
  ret i8 %value
}

define void @atomic_store16(ptr %p, i16 %value) {
; CHECK-LABEL: atomic_store16:
; CHECK:       st16
; CHECK-NOT:   __atomic
; CHECK:       ret
  store atomic i16 %value, ptr %p seq_cst, align 1
  ret void
}

define i32 @atomic_load32(ptr %p) {
; CHECK-LABEL: atomic_load32:
; CHECK:       ld32
; CHECK-NOT:   __atomic
; CHECK:       ret
  %value = load atomic i32, ptr %p seq_cst, align 1
  ret i32 %value
}

define i16 @atomic_add16(ptr %p, i16 %increment) {
; CHECK-LABEL: atomic_add16:
; CHECK:       ld16
; CHECK:       add
; CHECK:       st16
; CHECK-NOT:   __atomic
; CHECK:       ret
  %old = atomicrmw add ptr %p, i16 %increment seq_cst, align 1
  ret i16 %old
}

define { i16, i1 } @compare_exchange16(ptr %p, i16 %expected, i16 %desired) {
; CHECK-LABEL: compare_exchange16:
; CHECK:       ld16
; CHECK:       cmp
; CHECK:       st16
; CHECK-NOT:   __atomic
; CHECK:       ret
  %result = cmpxchg ptr %p, i16 %expected, i16 %desired seq_cst seq_cst,
                    align 1
  ret { i16, i1 } %result
}

define void @compiler_fence() {
; CHECK-LABEL: compiler_fence:
; CHECK:       ret
  fence seq_cst
  ret void
}

define i64 @atomic_load64(ptr %p) {
; CHECK-LABEL: atomic_load64:
; CHECK:       call __atomic_load
; CHECK:       ret
  %value = load atomic i64, ptr %p seq_cst, align 1
  ret i64 %value
}
