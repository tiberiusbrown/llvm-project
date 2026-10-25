; RUN: llc -mtriple=avm -O2 -verify-machineinstrs -stop-after=avm-aggregate-arguments %s -o - | FileCheck %s
; RUN: llc -mtriple=avm -O2 -avm-disable-aggregate-arguments -stop-after=avm-aggregate-arguments %s -o - | FileCheck %s --check-prefix=COPY
; RUN: llc -mtriple=avm -O0 -verify-machineinstrs %s -o %t
; RUN: llc -mtriple=avm -O2 -verify-machineinstrs %s -o %t

target datalayout = "e-p:16:8-p1:24:8-i8:8-i16:8-i32:8-i64:8-n8:16-S8-P1"
%item = type { i32, i16, i8, i8 }
%record = type { ptr addrspace(1), i16, i8 }
@items = internal global [16 x %item] zeroinitializer, align 1

; Borrow array elements directly, including the same object twice. The tail
; marker must be removed: other callers can pass objects in their stack frames.
define i32 @caller(i16 %index) {
; CHECK-LABEL: define i32 @caller(
; CHECK: %result = call addrspace(1) i32 @combine(ptr readonly align 1 %left, ptr readonly align 1 %right)
; COPY-LABEL: define i32 @caller(
; COPY: tail call addrspace(1) i32 @combine(ptr byval(%item) align 1 %left, ptr byval(%item) align 1 %right)
  %left = getelementptr [16 x %item], ptr @items, i16 0, i16 %index
  %next = add i16 %index, 5
  %wrapped = and i16 %next, 15
  %right = getelementptr [16 x %item], ptr @items, i16 0, i16 %wrapped
  %result = tail call i32 @combine(ptr byval(%item) align 1 %left, ptr byval(%item) align 1 %right)
  ret i32 %result
}

define i32 @caller_stack(i32 %value) {
; CHECK-LABEL: define i32 @caller_stack(
; CHECK: %result = call addrspace(1) i32 @combine(ptr readonly align 1 %local, ptr readonly align 1 %local)
  %local = alloca %item, align 1
  store %item zeroinitializer, ptr %local, align 1
  store i32 %value, ptr %local, align 1
  %result = tail call i32 @combine(ptr byval(%item) align 1 %local, ptr byval(%item) align 1 %local)
  ret i32 %result
}

define internal i32 @combine(ptr byval(%item) align 1 %left, ptr byval(%item) align 1 %right) noinline nosync memory(argmem: read) {
; CHECK-LABEL: define internal i32 @combine(ptr readonly align 1 %left, ptr readonly align 1 %right)
; COPY-LABEL: define internal i32 @combine(ptr byval(%item) align 1 %left, ptr byval(%item) align 1 %right)
  %a = load i32, ptr %left, align 1
  %enabled = getelementptr i8, ptr %right, i16 7
  %b = load i8, ptr %enabled, align 1
  %wide = zext i8 %b to i32
  %result = add i32 %a, %wide
  ret i32 %result
}

; Fields containing program pointers keep their original 24-bit representation.
define ptr addrspace(1) @caller_record(ptr %record) {
; CHECK-LABEL: define ptr addrspace(1) @caller_record(
; CHECK: call addrspace(1) ptr addrspace(1) @get_action(ptr readonly align 1 %record)
  %action = call ptr addrspace(1) @get_action(ptr byval(%record) align 1 %record)
  ret ptr addrspace(1) %action
}

define internal ptr addrspace(1) @get_action(ptr byval(%record) align 1 %record) noinline nosync memory(argmem: read) {
; CHECK-LABEL: define internal ptr addrspace(1) @get_action(ptr readonly align 1 %record)
; CHECK: load ptr addrspace(1), ptr %record, align 1
  %action = load ptr addrspace(1), ptr %record, align 1
  ret ptr addrspace(1) %action
}

; Specialize safe parameters individually, retaining a copy for identity uses.
define i32 @caller_partial(ptr %a, ptr %b, ptr %original) {
; CHECK-LABEL: define i32 @caller_partial(
; CHECK: @partial(ptr readonly align 1 %a, ptr byval(%item) align 1 %b, ptr %original)
  %result = call i32 @partial(ptr byval(%item) align 1 %a, ptr byval(%item) align 1 %b, ptr %original)
  ret i32 %result
}

define internal i32 @partial(ptr byval(%item) align 1 %a, ptr byval(%item) align 1 %b, ptr %original) noinline nosync memory(argmem: read) {
; CHECK-LABEL: define internal i32 @partial(ptr readonly align 1 %a, ptr byval(%item) align 1 %b, ptr %original)
  %value = load i32, ptr %a, align 1
  %same = icmp eq ptr %b, %original
  %bit = zext i1 %same to i32
  %result = add i32 %value, %bit
  ret i32 %result
}

; Even a memory(none) function can access its private byval copy. Broaden the
; function and an explicitly annotated call to include borrowed argument reads.
define i32 @caller_effects(ptr %a) {
; CHECK-LABEL: define i32 @caller_effects(
; CHECK: call addrspace(1) i32 @private_reads(ptr readonly align 1 %a) #[[READS:[0-9]+]]
  %result = call i32 @private_reads(ptr byval(%item) align 1 %a) memory(none)
  ret i32 %result
}

define internal i32 @private_reads(ptr byval(%item) align 1 %a) noinline nosync memory(none) {
; CHECK-LABEL: define internal i32 @private_reads(ptr readonly align 1 %a)
  %value = load i32, ptr %a, align 1
  ret i32 %value
}

; CHECK: attributes #[[READS]] = { memory(argmem: read) }
