; RUN: llc -mtriple=avm -O2 -stop-after=avm-aggregate-returns %s -o - | FileCheck %s
;
; These cases exercise the proof boundaries of the internal ABI specialization.
; Check both the caller and the callee, including all-or-nothing specialization.

target datalayout = "e-p:16:8-p1:24:8-i8:8-i16:8-i32:8-i64:8-n8:16-S8-P1"
%sample = type { i32, i16, i16, i8 }
%with_pointer = type { ptr, i32, i16, i8 }

declare void @capture(ptr)
declare void @observe(ptr captures(none)) nounwind willreturn
declare void @llvm.memcpy.p0.p0.i16(ptr, ptr, i16, i1 immarg)
declare ptr @llvm.invariant.start.p0(i64, ptr)

; Public definitions retain the C ABI.

define void @caller_external() {
; CHECK-LABEL: define void @caller_external(
; CHECK: call addrspace(1) void @step_external(ptr sret(%sample) %result, ptr byval(%sample) align 1 %current
; CHECK: @llvm.memcpy
  %current = alloca %sample, align 1
  %result = alloca %sample, align 1

  store %sample zeroinitializer, ptr %current, align 1

  call void @step_external(ptr sret(%sample) %result, ptr byval(%sample) align 1 %current)

  call void @llvm.memcpy.p0.p0.i16(ptr %current, ptr %result, i16 9, i1 false)

  ret void
}

define void @step_external(ptr sret(%sample) %out, ptr byval(%sample) align 1 %in) nounwind willreturn noinline {
; CHECK-LABEL: define void @step_external(ptr sret(%sample) %out, ptr byval(%sample) align 1 %in
  store i16 42, ptr %in, align 1
  call void @llvm.memcpy.p0.p0.i16(ptr %out, ptr %in, i16 9, i1 false)
  ret void
}

; A callback or global could observe changes before the assignment.

define void @caller_source_escapes() {
; CHECK-LABEL: define void @caller_source_escapes(
; CHECK: call addrspace(1) void @step_source_escapes(ptr sret(%sample) %result, ptr byval(%sample) align 1 %current
; CHECK: @llvm.memcpy
  %current = alloca %sample, align 1
  %result = alloca %sample, align 1

  store %sample zeroinitializer, ptr %current, align 1
  call void @capture(ptr %current)
  call void @step_source_escapes(ptr sret(%sample) %result, ptr byval(%sample) align 1 %current)

  call void @llvm.memcpy.p0.p0.i16(ptr %current, ptr %result, i16 9, i1 false)

  ret void
}

define internal void @step_source_escapes(ptr sret(%sample) %out, ptr byval(%sample) align 1 %in) nounwind willreturn noinline {
; CHECK-LABEL: define internal void @step_source_escapes(ptr sret(%sample) %out, ptr byval(%sample) align 1 %in
  store i16 42, ptr %in, align 1
  call void @llvm.memcpy.p0.p0.i16(ptr %out, ptr %in, i16 9, i1 false)
  ret void
}

; Another argument must not observe the object being overwritten.

define void @caller_alias_argument() {
; CHECK-LABEL: define void @caller_alias_argument(
; CHECK: call addrspace(1) void @step_alias_argument(ptr sret(%sample) %result, ptr byval(%sample) align 1 %current
; CHECK: @llvm.memcpy
  %current = alloca %sample, align 1
  %result = alloca %sample, align 1

  store %sample zeroinitializer, ptr %current, align 1

  call void @step_alias_argument(ptr sret(%sample) %result, ptr byval(%sample) align 1 %current, ptr %current)

  call void @llvm.memcpy.p0.p0.i16(ptr %current, ptr %result, i16 9, i1 false)

  ret void
}

define internal void @step_alias_argument(ptr sret(%sample) %out, ptr byval(%sample) align 1 %in, ptr %alias) nounwind willreturn noinline {
; CHECK-LABEL: define internal void @step_alias_argument(ptr sret(%sample) %out, ptr byval(%sample) align 1 %in
  %old = load i16, ptr %alias, align 1
  store i16 %old, ptr %in, align 1
  call void @llvm.memcpy.p0.p0.i16(ptr %out, ptr %in, i16 9, i1 false)
  ret void
}

; The sret storage must be a private temporary.

define void @caller_result_escapes() {
; CHECK-LABEL: define void @caller_result_escapes(
; CHECK: call addrspace(1) void @step_result_escapes(ptr sret(%sample) %result, ptr byval(%sample) align 1 %current
; CHECK: @llvm.memcpy
  %current = alloca %sample, align 1
  %result = alloca %sample, align 1

  store %sample zeroinitializer, ptr %current, align 1
  call void @capture(ptr %result)
  call void @step_result_escapes(ptr sret(%sample) %result, ptr byval(%sample) align 1 %current)

  call void @llvm.memcpy.p0.p0.i16(ptr %current, ptr %result, i16 9, i1 false)

  ret void
}

define internal void @step_result_escapes(ptr sret(%sample) %out, ptr byval(%sample) align 1 %in) nounwind willreturn noinline {
; CHECK-LABEL: define internal void @step_result_escapes(ptr sret(%sample) %out, ptr byval(%sample) align 1 %in
  store i16 42, ptr %in, align 1
  call void @llvm.memcpy.p0.p0.i16(ptr %out, ptr %in, i16 9, i1 false)
  ret void
}

; No result access may intervene before the copy back.

define void @caller_result_observed() {
; CHECK-LABEL: define void @caller_result_observed(
; CHECK: call addrspace(1) void @step_result_observed(ptr sret(%sample) %result, ptr byval(%sample) align 1 %current
; CHECK: @llvm.memcpy
  %current = alloca %sample, align 1
  %result = alloca %sample, align 1

  store %sample zeroinitializer, ptr %current, align 1

  call void @step_result_observed(ptr sret(%sample) %result, ptr byval(%sample) align 1 %current)
  %observed = load volatile i8, ptr %result, align 1
  call void @llvm.memcpy.p0.p0.i16(ptr %current, ptr %result, i16 9, i1 false)

  ret void
}

define internal void @step_result_observed(ptr sret(%sample) %out, ptr byval(%sample) align 1 %in) nounwind willreturn noinline {
; CHECK-LABEL: define internal void @step_result_observed(ptr sret(%sample) %out, ptr byval(%sample) align 1 %in
  store i16 42, ptr %in, align 1
  call void @llvm.memcpy.p0.p0.i16(ptr %out, ptr %in, i16 9, i1 false)
  ret void
}

; Returning into another object preserves the original input.

define void @caller_retained_input() {
; CHECK-LABEL: define void @caller_retained_input(
; CHECK: call addrspace(1) void @step_retained_input(ptr sret(%sample) %result, ptr byval(%sample) align 1 %current
; CHECK: @llvm.memcpy
  %current = alloca %sample, align 1
  %result = alloca %sample, align 1
  %other = alloca %sample, align 1
  store %sample zeroinitializer, ptr %current, align 1

  call void @step_retained_input(ptr sret(%sample) %result, ptr byval(%sample) align 1 %current)

  call void @llvm.memcpy.p0.p0.i16(ptr %other, ptr %result, i16 9, i1 false)
  %old = load volatile i8, ptr %current, align 1
  ret void
}

define internal void @step_retained_input(ptr sret(%sample) %out, ptr byval(%sample) align 1 %in) nounwind willreturn noinline {
; CHECK-LABEL: define internal void @step_retained_input(ptr sret(%sample) %out, ptr byval(%sample) align 1 %in
  store i16 42, ptr %in, align 1
  call void @llvm.memcpy.p0.p0.i16(ptr %out, ptr %in, i16 9, i1 false)
  ret void
}

; Even a noncapturing call could inspect the parameter's identity.

define void @caller_input_escapes() {
; CHECK-LABEL: define void @caller_input_escapes(
; CHECK: call addrspace(1) void @step_input_escapes(ptr sret(%sample) %result, ptr byval(%sample) align 1 %current
; CHECK: @llvm.memcpy
  %current = alloca %sample, align 1
  %result = alloca %sample, align 1

  store %sample zeroinitializer, ptr %current, align 1

  call void @step_input_escapes(ptr sret(%sample) %result, ptr byval(%sample) align 1 %current)

  call void @llvm.memcpy.p0.p0.i16(ptr %current, ptr %result, i16 9, i1 false)

  ret void
}

define internal void @step_input_escapes(ptr sret(%sample) %out, ptr byval(%sample) align 1 %in) nounwind willreturn noinline {
; CHECK-LABEL: define internal void @step_input_escapes(ptr sret(%sample) %out, ptr byval(%sample) align 1 %in
  call void @observe(ptr %in)
  call void @llvm.memcpy.p0.p0.i16(ptr %out, ptr %in, i16 9, i1 false)
  ret void
}

; The private byval parameter has a different address from the source.

define void @caller_input_identity() {
; CHECK-LABEL: define void @caller_input_identity(
; CHECK: call addrspace(1) void @step_input_identity(ptr sret(%sample) %result, ptr byval(%sample) align 1 %current
; CHECK: @llvm.memcpy
  %current = alloca %sample, align 1
  %result = alloca %sample, align 1

  store %sample zeroinitializer, ptr %current, align 1

  call void @step_input_identity(ptr sret(%sample) %result, ptr byval(%sample) align 1 %current)

  call void @llvm.memcpy.p0.p0.i16(ptr %current, ptr %result, i16 9, i1 false)

  ret void
}

define internal void @step_input_identity(ptr sret(%sample) %out, ptr byval(%sample) align 1 %in) nounwind willreturn noinline {
; CHECK-LABEL: define internal void @step_input_identity(ptr sret(%sample) %out, ptr byval(%sample) align 1 %in
  %address = ptrtoint ptr %in to i16
  store i16 %address, ptr %in, align 1
  call void @llvm.memcpy.p0.p0.i16(ptr %out, ptr %in, i16 9, i1 false)
  ret void
}

; Do not redirect volatile accesses.

define void @caller_volatile_input() {
; CHECK-LABEL: define void @caller_volatile_input(
; CHECK: call addrspace(1) void @step_volatile_input(ptr sret(%sample) %result, ptr byval(%sample) align 1 %current
; CHECK: @llvm.memcpy
  %current = alloca %sample, align 1
  %result = alloca %sample, align 1

  store %sample zeroinitializer, ptr %current, align 1

  call void @step_volatile_input(ptr sret(%sample) %result, ptr byval(%sample) align 1 %current)

  call void @llvm.memcpy.p0.p0.i16(ptr %current, ptr %result, i16 9, i1 false)

  ret void
}

define internal void @step_volatile_input(ptr sret(%sample) %out, ptr byval(%sample) align 1 %in) nounwind willreturn noinline {
; CHECK-LABEL: define internal void @step_volatile_input(ptr sret(%sample) %out, ptr byval(%sample) align 1 %in
  %v = load volatile i16, ptr %in, align 1
  store i16 %v, ptr %in, align 1
  call void @llvm.memcpy.p0.p0.i16(ptr %out, ptr %in, i16 9, i1 false)
  ret void
}

; Do not redirect atomic accesses.

define void @caller_atomic_input() {
; CHECK-LABEL: define void @caller_atomic_input(
; CHECK: call addrspace(1) void @step_atomic_input(ptr sret(%sample) %result, ptr byval(%sample) align 1 %current
; CHECK: @llvm.memcpy
  %current = alloca %sample, align 1
  %result = alloca %sample, align 1

  store %sample zeroinitializer, ptr %current, align 1

  call void @step_atomic_input(ptr sret(%sample) %result, ptr byval(%sample) align 1 %current)

  call void @llvm.memcpy.p0.p0.i16(ptr %current, ptr %result, i16 9, i1 false)

  ret void
}

define internal void @step_atomic_input(ptr sret(%sample) %out, ptr byval(%sample) align 1 %in) nounwind willreturn noinline {
; CHECK-LABEL: define internal void @step_atomic_input(ptr sret(%sample) %out, ptr byval(%sample) align 1 %in
  %v = load atomic i8, ptr %in unordered, align 1
  store i8 %v, ptr %in, align 1
  call void @llvm.memcpy.p0.p0.i16(ptr %out, ptr %in, i16 9, i1 false)
  ret void
}

; An invariant on a private parameter must not freeze the caller object.

define void @caller_input_invariant() {
; CHECK-LABEL: define void @caller_input_invariant(
; CHECK: call addrspace(1) void @step_input_invariant(ptr sret(%sample) %result, ptr byval(%sample) align 1 %current
; CHECK: @llvm.memcpy
  %current = alloca %sample, align 1
  %result = alloca %sample, align 1

  store %sample zeroinitializer, ptr %current, align 1

  call void @step_input_invariant(ptr sret(%sample) %result, ptr byval(%sample) align 1 %current)

  call void @llvm.memcpy.p0.p0.i16(ptr %current, ptr %result, i16 9, i1 false)

  ret void
}

define internal void @step_input_invariant(ptr sret(%sample) %out, ptr byval(%sample) align 1 %in) nounwind willreturn noinline {
; CHECK-LABEL: define internal void @step_input_invariant(ptr sret(%sample) %out, ptr byval(%sample) align 1 %in
  %region = call ptr @llvm.invariant.start.p0(i64 9, ptr %in)
  call void @llvm.memcpy.p0.p0.i16(ptr %out, ptr %in, i16 9, i1 false)
  ret void
}

; Returning fewer than all bytes cannot coalesce whole objects.

define void @caller_partial_return() {
; CHECK-LABEL: define void @caller_partial_return(
; CHECK: call addrspace(1) void @step_partial_return(ptr sret(%sample) %result, ptr byval(%sample) align 1 %current
; CHECK: @llvm.memcpy
  %current = alloca %sample, align 1
  %result = alloca %sample, align 1

  store %sample zeroinitializer, ptr %current, align 1

  call void @step_partial_return(ptr sret(%sample) %result, ptr byval(%sample) align 1 %current)

  call void @llvm.memcpy.p0.p0.i16(ptr %current, ptr %result, i16 9, i1 false)

  ret void
}

define internal void @step_partial_return(ptr sret(%sample) %out, ptr byval(%sample) align 1 %in) nounwind willreturn noinline {
; CHECK-LABEL: define internal void @step_partial_return(ptr sret(%sample) %out, ptr byval(%sample) align 1 %in
  store i16 42, ptr %in, align 1
  call void @llvm.memcpy.p0.p0.i16(ptr %out, ptr %in, i16 8, i1 false)
  ret void
}

; Every returning path must copy the same complete input.

define void @caller_early_return() {
; CHECK-LABEL: define void @caller_early_return(
; CHECK: call addrspace(1) void @step_early_return(ptr sret(%sample) %result, ptr byval(%sample) align 1 %current
; CHECK: @llvm.memcpy
  %current = alloca %sample, align 1
  %result = alloca %sample, align 1

  store %sample zeroinitializer, ptr %current, align 1

  call void @step_early_return(ptr sret(%sample) %result, ptr byval(%sample) align 1 %current, i1 true)

  call void @llvm.memcpy.p0.p0.i16(ptr %current, ptr %result, i16 9, i1 false)

  ret void
}

define internal void @step_early_return(ptr sret(%sample) %out, ptr byval(%sample) align 1 %in, i1 %choose) nounwind willreturn noinline {
; CHECK-LABEL: define internal void @step_early_return(ptr sret(%sample) %out, ptr byval(%sample) align 1 %in
  br i1 %choose, label %copy, label %early
early:
  ret void
copy:
  call void @llvm.memcpy.p0.p0.i16(ptr %out, ptr %in, i16 9, i1 false)
  ret void
}

; An unwind could make the old source visible after partial modification.

define void @caller_may_unwind() {
; CHECK-LABEL: define void @caller_may_unwind(
; CHECK: call addrspace(1) void @step_may_unwind(ptr sret(%sample) %result, ptr byval(%sample) align 1 %current
; CHECK: @llvm.memcpy
  %current = alloca %sample, align 1
  %result = alloca %sample, align 1

  store %sample zeroinitializer, ptr %current, align 1

  call void @step_may_unwind(ptr sret(%sample) %result, ptr byval(%sample) align 1 %current)

  call void @llvm.memcpy.p0.p0.i16(ptr %current, ptr %result, i16 9, i1 false)

  ret void
}

define internal void @step_may_unwind(ptr sret(%sample) %out, ptr byval(%sample) align 1 %in) willreturn noinline {
; CHECK-LABEL: define internal void @step_may_unwind(ptr sret(%sample) %out, ptr byval(%sample) align 1 %in
  store i16 42, ptr %in, align 1
  call void @llvm.memcpy.p0.p0.i16(ptr %out, ptr %in, i16 9, i1 false)
  ret void
}

; Without willreturn a non-local exit could expose partial changes.

define void @caller_nonlocal_exit() {
; CHECK-LABEL: define void @caller_nonlocal_exit(
; CHECK: call addrspace(1) void @step_nonlocal_exit(ptr sret(%sample) %result, ptr byval(%sample) align 1 %current
; CHECK: @llvm.memcpy
  %current = alloca %sample, align 1
  %result = alloca %sample, align 1

  store %sample zeroinitializer, ptr %current, align 1

  call void @step_nonlocal_exit(ptr sret(%sample) %result, ptr byval(%sample) align 1 %current)

  call void @llvm.memcpy.p0.p0.i16(ptr %current, ptr %result, i16 9, i1 false)

  ret void
}

define internal void @step_nonlocal_exit(ptr sret(%sample) %out, ptr byval(%sample) align 1 %in) nounwind noinline {
; CHECK-LABEL: define internal void @step_nonlocal_exit(ptr sret(%sample) %out, ptr byval(%sample) align 1 %in
  store i16 42, ptr %in, align 1
  call void @llvm.memcpy.p0.p0.i16(ptr %out, ptr %in, i16 9, i1 false)
  ret void
}

; Preserve calls with operand bundles.

define void @caller_operand_bundle() {
; CHECK-LABEL: define void @caller_operand_bundle(
; CHECK: call addrspace(1) void @step_operand_bundle(ptr sret(%sample) %result, ptr byval(%sample) align 1 %current
; CHECK: @llvm.memcpy
  %current = alloca %sample, align 1
  %result = alloca %sample, align 1

  store %sample zeroinitializer, ptr %current, align 1

  call void @step_operand_bundle(ptr sret(%sample) %result, ptr byval(%sample) align 1 %current) [ "deopt"(i16 0) ]

  call void @llvm.memcpy.p0.p0.i16(ptr %current, ptr %result, i16 9, i1 false)

  ret void
}

define internal void @step_operand_bundle(ptr sret(%sample) %out, ptr byval(%sample) align 1 %in) nounwind willreturn noinline {
; CHECK-LABEL: define internal void @step_operand_bundle(ptr sret(%sample) %out, ptr byval(%sample) align 1 %in
  store i16 42, ptr %in, align 1
  call void @llvm.memcpy.p0.p0.i16(ptr %out, ptr %in, i16 9, i1 false)
  ret void
}

; The hidden byval copy can be more aligned than the source alloca.

define void @caller_strong_alignment() {
; CHECK-LABEL: define void @caller_strong_alignment(
; CHECK: call addrspace(1) void @step_strong_alignment(ptr sret(%sample) %result, ptr byval(%sample) align 4 %current
; CHECK: @llvm.memcpy
  %current = alloca %sample, align 1
  %result = alloca %sample, align 1

  store %sample zeroinitializer, ptr %current, align 1

  call void @step_strong_alignment(ptr sret(%sample) %result, ptr byval(%sample) align 4 %current)

  call void @llvm.memcpy.p0.p0.i16(ptr %current, ptr %result, i16 9, i1 false)

  ret void
}

define internal void @step_strong_alignment(ptr sret(%sample) %out, ptr byval(%sample) align 4 %in) nounwind willreturn noinline {
; CHECK-LABEL: define internal void @step_strong_alignment(ptr sret(%sample) %out, ptr byval(%sample) align 4 %in
  store i16 42, ptr %in, align 1
  call void @llvm.memcpy.p0.p0.i16(ptr %out, ptr %in, i16 9, i1 false)
  ret void
}

; An indirect call must continue to use the original ABI.
@function_pointer = global ptr addrspace(1) @step_address_taken
define void @caller_address_taken() {
; CHECK-LABEL: define void @caller_address_taken(
; CHECK: call addrspace(1) void @step_address_taken(ptr sret(%sample) %result, ptr byval(%sample) align 1 %current
; CHECK: @llvm.memcpy
  %current = alloca %sample, align 1
  %result = alloca %sample, align 1

  store %sample zeroinitializer, ptr %current, align 1

  call void @step_address_taken(ptr sret(%sample) %result, ptr byval(%sample) align 1 %current)

  call void @llvm.memcpy.p0.p0.i16(ptr %current, ptr %result, i16 9, i1 false)

  ret void
}

define internal void @step_address_taken(ptr sret(%sample) %out, ptr byval(%sample) align 1 %in) nounwind willreturn noinline {
; CHECK-LABEL: define internal void @step_address_taken(ptr sret(%sample) %out, ptr byval(%sample) align 1 %in
  store i16 42, ptr %in, align 1
  call void @llvm.memcpy.p0.p0.i16(ptr %out, ptr %in, i16 9, i1 false)
  ret void
}

; One unsafe caller prevents specialization of the whole function.

define void @caller_mixed_callers() {
; CHECK-LABEL: define void @caller_mixed_callers(
; CHECK: call addrspace(1) void @step_mixed_callers(ptr sret(%sample) %result, ptr byval(%sample) align 1 %current
; CHECK: @llvm.memcpy
  %current = alloca %sample, align 1
  %result = alloca %sample, align 1

  store %sample zeroinitializer, ptr %current, align 1

  call void @step_mixed_callers(ptr sret(%sample) %result, ptr byval(%sample) align 1 %current)

  call void @llvm.memcpy.p0.p0.i16(ptr %current, ptr %result, i16 9, i1 false)

  ret void
}

define internal void @step_mixed_callers(ptr sret(%sample) %out, ptr byval(%sample) align 1 %in) nounwind willreturn noinline {
; CHECK-LABEL: define internal void @step_mixed_callers(ptr sret(%sample) %out, ptr byval(%sample) align 1 %in
  store i16 42, ptr %in, align 1
  call void @llvm.memcpy.p0.p0.i16(ptr %out, ptr %in, i16 9, i1 false)
  ret void
}

define void @other_caller_mixed_callers(ptr %destination) {
; CHECK-LABEL: define void @other_caller_mixed_callers(
; CHECK: call addrspace(1) void @step_mixed_callers(ptr sret(%sample) %destination, ptr byval(%sample) align 1 %current)
  %current = alloca %sample, align 1
  store %sample zeroinitializer, ptr %current, align 1
  call void @step_mixed_callers(ptr sret(%sample) %destination, ptr byval(%sample) align 1 %current)
  ret void
}

; A self pointer stored in the aggregate can reveal its source address.

define void @caller_pointer_member() {
; CHECK-LABEL: define void @caller_pointer_member(
; CHECK: call addrspace(1) void @step_pointer_member(ptr sret(%with_pointer) %result, ptr byval(%with_pointer) align 1 %current
; CHECK: @llvm.memcpy
  %current = alloca %with_pointer, align 1
  %result = alloca %with_pointer, align 1

  store %with_pointer zeroinitializer, ptr %current, align 1
  store ptr %current, ptr %current, align 1
  call void @step_pointer_member(ptr sret(%with_pointer) %result, ptr byval(%with_pointer) align 1 %current)

  call void @llvm.memcpy.p0.p0.i16(ptr %current, ptr %result, i16 9, i1 false)

  ret void
}

define internal void @step_pointer_member(ptr sret(%with_pointer) %out, ptr byval(%with_pointer) align 1 %in) nounwind willreturn noinline {
; CHECK-LABEL: define internal void @step_pointer_member(ptr sret(%with_pointer) %out, ptr byval(%with_pointer) align 1 %in
  store i16 42, ptr %in, align 1
  call void @llvm.memcpy.p0.p0.i16(ptr %out, ptr %in, i16 9, i1 false)
  ret void
}
