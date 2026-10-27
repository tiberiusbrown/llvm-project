; RUN: llc -mtriple=avm -O2 -verify-machineinstrs < %s | FileCheck %s
; RUN: llc -mtriple=avm -O2 -verify-machineinstrs -filetype=obj %s -o %t.o
; RUN: opt -mtriple=avm -passes='default<O2>' -S %s | llc -mtriple=avm -O2 -verify-machineinstrs | FileCheck %s
;
; SimplifyCFG is allowed to speculate one expensive instruction. Recover a
; profitable branch for bounded return selects after physical allocation,
; without lengthening live ranges or changing cheap CMOVs.

define float @conditional_div(float %a, float %b, i16 %condition) {
; CHECK-LABEL: conditional_div:
; CHECK:       br{{eq|ne}} [[END:[.a-zA-Z0-9_]+]]
; CHECK:       fdiv
; CHECK:       [[END]]:
; CHECK:       ret
  %c = icmp ne i16 %condition, 0
  %d = fdiv float %a, %b
  %r = select i1 %c, float %d, float %a
  ret float %r
}

define float @reciprocal(float %a, i16 %condition) {
; CHECK-LABEL: reciprocal:
; CHECK:       br{{eq|ne}} [[END:[.a-zA-Z0-9_]+]]
; CHECK:       fdiv
; CHECK:       [[END]]:
; CHECK:       ret
  %c = icmp ne i16 %condition, 0
  %d = fdiv float -1.0, %a
  %r = select i1 %c, float %d, float %a
  ret float %r
}

define float @constant_divisor(float %a, i16 %condition) {
; CHECK-LABEL: constant_divisor:
; CHECK:       br{{eq|ne}}
; CHECK:       fdiv
; CHECK-NOT:   cmov
; CHECK:       ret
  %c = icmp ne i16 %condition, 0
  %d = fdiv float %a, 3.0
  %r = select i1 %c, float %d, float %a
  ret float %r
}

; Exact reciprocal constants become a cheap FMUL in InstCombine/DAGCombine.
define float @power_of_two_divisor(float %a, i16 %condition) {
; CHECK-LABEL: power_of_two_divisor:
; CHECK-NOT:   br{{eq|ne}}
; CHECK:       fmul
; CHECK:       cmov
; CHECK:       ret
  %c = icmp ne i16 %condition, 0
  %d = fdiv float %a, 2.0
  %r = select i1 %c, float %d, float %a
  ret float %r
}

; The division is needed regardless of the selection: it has another user.
define float @division_on_both_paths(float %a, float %b, i16 %condition) {
; CHECK-LABEL: division_on_both_paths:
; CHECK-NOT:   br{{eq|ne}}
; CHECK:       fdiv
; CHECK:       cmov
; CHECK:       fadd
; CHECK:       ret
  %c = icmp ne i16 %condition, 0
  %d = fdiv float %a, %b
  %r = select i1 %c, float %d, float %a
  %sum = fadd float %r, %d
  ret float %sum
}

define float @live_across_branch(float %a, float %b, i16 %condition) {
; CHECK-LABEL: live_across_branch:
; CHECK:       br{{eq|ne}}
; CHECK:       fdiv
; CHECK:       fadd
; CHECK:       ret
  %c = icmp ne i16 %condition, 0
  %d = fdiv float %a, %b
  %r = select i1 %c, float %d, float %a
  %sum = fadd float %r, %b
  ret float %sum
}

; A logical condition need not be a single-use CmpInst to skip expensive work.
define float @nested_conditions(float %a, i16 %c, i16 %d) {
; CHECK-LABEL: nested_conditions:
; CHECK:       br{{eq|ne}}
; CHECK:       fdiv
; CHECK:       ret
  %both = and i16 %c, %d
  %condition = icmp ne i16 %both, 0
  %quotient = fdiv float -1.0, %a
  %r = select i1 %condition, float %quotient, float %a
  ret float %r
}

define i16 @conditional_integer_div(i16 %a, i16 %b, i16 %condition) {
; CHECK-LABEL: conditional_integer_div:
; CHECK:       br{{eq|ne}} [[END:[.a-zA-Z0-9_]+]]
; CHECK:       sdiv16
; CHECK:       [[END]]:
; CHECK:       ret
  %c = icmp ne i16 %condition, 0
  br i1 %c, label %divide, label %end
divide:
  %d = sdiv i16 %a, %b
  br label %end
end:
  %r = phi i16 [ %a, %0 ], [ %d, %divide ]
  ret i16 %r
}

; Native integer division is cheaper than FDIV; do not change its speculation
; policy without evidence that the extra control flow pays for itself.
define i16 @constant_integer_div(i16 %a, i16 %condition) {
; CHECK-LABEL: constant_integer_div:
; CHECK-NOT:   br{{eq|ne}}
; CHECK:       sdiv16
; CHECK:       cmov
; CHECK:       ret
  %c = icmp ne i16 %condition, 0
  %d = sdiv i16 %a, 7
  %r = select i1 %c, i16 %d, i16 %a
  ret i16 %r
}

define i16 @cheap_arithmetic(i16 %a, i16 %b, i16 %condition) {
; CHECK-LABEL: cheap_arithmetic:
; CHECK-NOT:   br{{eq|ne}}
; CHECK-DAG:   add
; CHECK-DAG:   cmov
; CHECK:       ret
  %c = icmp ne i16 %condition, 0
  %d = add i16 %a, %b
  %r = select i1 %c, i16 %d, i16 %a
  ret i16 %r
}

define float @cheap_float_arithmetic(float %a, float %b, i16 %condition) {
; CHECK-LABEL: cheap_float_arithmetic:
; CHECK-NOT:   br{{eq|ne}}
; CHECK:       fadd
; CHECK:       cmov
; CHECK:       ret
  %c = icmp ne i16 %condition, 0
  %d = fadd float %a, %b
  %r = select i1 %c, float %d, float %a
  ret float %r
}

define i32 @shared_constant_word(i16 %condition) {
; CHECK-LABEL: shared_constant_word:
; CHECK-NOT:   br{{eq|ne}}
; CHECK:       cmov
; CHECK-NOT:   cmov
; CHECK:       ret
  %c = icmp ne i16 %condition, 0
  %r = select i1 %c, i32 305419896, i32 -1698933128
  ret i32 %r
}

define float @conditional_remainder(float %a, float %b, i16 %condition) {
; CHECK-LABEL: conditional_remainder:
; CHECK-NOT:   br{{eq|ne}}
; CHECK:       fmod
; CHECK:       cmov
; CHECK:       ret
  %c = icmp ne i16 %condition, 0
  %d = frem float %a, %b
  %r = select i1 %c, float %d, float %a
  ret float %r
}

declare float @llvm.sqrt.f32(float)

define float @conditional_helper(float %a, i16 %condition) {
; CHECK-LABEL: conditional_helper:
; CHECK-NOT:   br{{eq|ne}}
; CHECK:       fsqrt
; CHECK:       cmov
; CHECK:       ret
  %c = icmp ne i16 %condition, 0
  %d = call float @llvm.sqrt.f32(float %a)
  %r = select i1 %c, float %d, float %a
  ret float %r
}

; Both outcomes improve, so even a highly predictable division qualifies.
define float @hot_division(float %a, float %b, i16 %condition) {
; CHECK-LABEL: hot_division:
; CHECK:       br{{eq|ne}}
; CHECK:       fdiv
; CHECK:       ret
  %c = icmp ne i16 %condition, 0
  %d = fdiv float %a, %b
  %r = select i1 %c, float %d, float %a, !prof !0
  ret float %r
}

; High predictability alone must not turn an inexpensive constant CMOV into
; control flow on an interpreter without a hardware branch predictor.
define i32 @predictable_constant(float %unused, i16 %condition) {
; CHECK-LABEL: predictable_constant:
; CHECK-NOT:   br{{eq|ne}}
; CHECK:       cmov
; CHECK-NOT:   cmov
; CHECK:       ret
  %c = icmp ne i16 %condition, 0
  %r = select i1 %c, i32 305419896, i32 -1698933128, !prof !0
  ret i32 %r
}

!0 = !{!"branch_weights", i32 2000, i32 1}
