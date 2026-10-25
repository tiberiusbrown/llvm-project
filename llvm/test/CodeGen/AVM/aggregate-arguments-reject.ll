; RUN: llc -mtriple=avm -O2 -verify-machineinstrs -stop-after=avm-aggregate-arguments %s -o - | FileCheck %s

target datalayout = "e-p:16:8-p1:24:8-i8:8-i16:8-i32:8-i64:8-n8:16-S8-P1"
%item = type { i32, i16, i8, i8 }
@object = global %item zeroinitializer, align 1
@escaped = global ptr addrspace(1) @address_taken
declare void @observe(ptr captures(none)) nosync memory(read)
declare void @callback()

define void @caller(ptr %a, ptr %alias, i16 %index) {
  call i32 @external(ptr byval(%item) align 1 %a)
  call i32 @address_taken(ptr byval(%item) align 1 %a)
  call i32 @write_alias(ptr byval(%item) align 1 %a, ptr %alias)
  call i32 @write_global(ptr byval(%item) align 1 %a)
  call i32 @with_callback(ptr byval(%item) align 1 %a)
  call i32 @mutate_copy(ptr byval(%item) align 1 %a)
  call i1 @identity(ptr byval(%item) align 1 %a, ptr %alias)
  call i32 @escape(ptr byval(%item) align 1 %a)
  call i32 @forward(ptr byval(%item) align 1 %a)
  call i32 @volatile_load(ptr byval(%item) align 1 %a)
  call i32 @atomic_load(ptr byval(%item) align 1 %a)
  call i32 @dynamic_offset(ptr byval(%item) align 1 %a, i16 %index)
  call i32 @overrun(ptr byval(%item) align 1 %a)
  call i32 @negative_offset(ptr byval(%item) align 1 %a)
  call i32 @synchronizes(ptr byval(%item) align 1 %a)
  call i32 @explicit_noalias(ptr byval(%item) align 1 %a)
  call i32 @optnone(ptr byval(%item) align 1 %a)
  call i32 @inconsistent(ptr byval(%item) align 1 %a)
  call i32 @inconsistent(ptr %a)
  call i32 @strong_alignment(ptr byval(%item) align 1 %a)
  call i32 @outgoing_musttail(ptr byval(%item) align 1 %a)
  call i32 @bundled(ptr byval(%item) align 1 %a) [ "deopt"(i16 0) ]
  ret void
}

define i32 @external(ptr byval(%item) align 1 %a) nosync memory(read) {
; CHECK-LABEL: define i32 @external(ptr byval(%item) align 1 %a)
  %value = load i32, ptr %a, align 1
  ret i32 %value
}

define internal i32 @address_taken(ptr byval(%item) align 1 %a) nosync memory(read) {
; CHECK-LABEL: define internal i32 @address_taken(ptr byval(%item) align 1 %a)
  %value = load i32, ptr %a, align 1
  ret i32 %value
}

define internal i32 @write_alias(ptr byval(%item) align 1 %a, ptr %alias) nosync {
; CHECK-LABEL: define internal i32 @write_alias(ptr byval(%item) align 1 %a,
  store i32 42, ptr %alias, align 1
  %value = load i32, ptr %a, align 1
  ret i32 %value
}

define internal i32 @write_global(ptr byval(%item) align 1 %a) nosync {
; CHECK-LABEL: define internal i32 @write_global(ptr byval(%item) align 1 %a)
  store i32 42, ptr @object, align 1
  %value = load i32, ptr %a, align 1
  ret i32 %value
}

define internal i32 @with_callback(ptr byval(%item) align 1 %a) {
; CHECK-LABEL: define internal i32 @with_callback(ptr byval(%item) align 1 %a)
  call void @callback()
  %value = load i32, ptr %a, align 1
  ret i32 %value
}

define internal i32 @mutate_copy(ptr byval(%item) align 1 %a) nosync {
; CHECK-LABEL: define internal i32 @mutate_copy(ptr byval(%item) align 1 %a)
  store i32 42, ptr %a, align 1
  ret i32 42
}

define internal i1 @identity(ptr byval(%item) align 1 %a, ptr %alias) nosync memory(none) {
; CHECK-LABEL: define internal i1 @identity(ptr byval(%item) align 1 %a,
  %same = icmp eq ptr %a, %alias
  ret i1 %same
}

define internal i32 @escape(ptr byval(%item) align 1 %a) nosync memory(none) {
; CHECK-LABEL: define internal i32 @escape(ptr byval(%item) align 1 %a)
  %address = ptrtoint ptr %a to i32
  ret i32 %address
}

define internal i32 @forward(ptr byval(%item) align 1 %a) nosync memory(read) {
; CHECK-LABEL: define internal i32 @forward(ptr byval(%item) align 1 %a)
  call void @observe(ptr %a)
  %value = load i32, ptr %a, align 1
  ret i32 %value
}

define internal i32 @volatile_load(ptr byval(%item) align 1 %a) nosync memory(read) {
; CHECK-LABEL: define internal i32 @volatile_load(ptr byval(%item) align 1 %a)
  %value = load volatile i32, ptr %a, align 1
  ret i32 %value
}

define internal i32 @atomic_load(ptr byval(%item) align 1 %a) nosync memory(read) {
; CHECK-LABEL: define internal i32 @atomic_load(ptr byval(%item) align 1 %a)
  %value = load atomic i32, ptr %a monotonic, align 1
  ret i32 %value
}

define internal i32 @dynamic_offset(ptr byval(%item) align 1 %a, i16 %index) nosync memory(read) {
; CHECK-LABEL: define internal i32 @dynamic_offset(ptr byval(%item) align 1 %a,
  %offset = getelementptr i8, ptr %a, i16 %index
  %value = load i32, ptr %offset, align 1
  ret i32 %value
}

define internal i32 @overrun(ptr byval(%item) align 1 %a) nosync memory(read) {
; CHECK-LABEL: define internal i32 @overrun(ptr byval(%item) align 1 %a)
  %offset = getelementptr i8, ptr %a, i16 7
  %value = load i32, ptr %offset, align 1
  ret i32 %value
}

define internal i32 @negative_offset(ptr byval(%item) align 1 %a) nosync memory(read) {
; CHECK-LABEL: define internal i32 @negative_offset(ptr byval(%item) align 1 %a)
  %offset = getelementptr i8, ptr %a, i16 -1
  %value = load i32, ptr %offset, align 1
  ret i32 %value
}

define internal i32 @synchronizes(ptr byval(%item) align 1 %a) memory(read) {
; CHECK-LABEL: define internal i32 @synchronizes(ptr byval(%item) align 1 %a)
  %value = load i32, ptr %a, align 1
  ret i32 %value
}

define internal i32 @explicit_noalias(ptr noalias byval(%item) align 1 %a) nosync memory(read) {
; CHECK-LABEL: define internal i32 @explicit_noalias(ptr noalias byval(%item) align 1 %a)
  %value = load i32, ptr %a, align 1
  ret i32 %value
}

define internal i32 @optnone(ptr byval(%item) align 1 %a) noinline optnone nosync memory(read) {
; CHECK-LABEL: define internal i32 @optnone(ptr byval(%item) align 1 %a)
  %value = load i32, ptr %a, align 1
  ret i32 %value
}

define internal i32 @inconsistent(ptr byval(%item) align 1 %a) nosync memory(read) {
; CHECK-LABEL: define internal i32 @inconsistent(ptr byval(%item) align 1 %a)
  %value = load i32, ptr %a, align 1
  ret i32 %value
}

define internal i32 @strong_alignment(ptr byval(%item) align 2 %a) nosync memory(read) {
; CHECK-LABEL: define internal i32 @strong_alignment(ptr byval(%item) align 2 %a)
  %value = load i32, ptr %a, align 2
  ret i32 %value
}

; A musttail caller fixes the ABI for every caller of this function.
define i32 @tail_caller(ptr byval(%item) align 1 %a) {
  %value = musttail call i32 @tail_target(ptr byval(%item) align 1 %a)
  ret i32 %value
}

define internal i32 @tail_target(ptr byval(%item) align 1 %a) nosync memory(read) {
; CHECK-LABEL: define internal i32 @tail_target(ptr byval(%item) align 1 %a)
  %value = load i32, ptr %a, align 1
  ret i32 %value
}

; The candidate only has loads, but the outgoing musttail call fixes its ABI.
declare i32 @tail_sink(ptr byval(%item) align 1) nosync memory(read)
define internal i32 @outgoing_musttail(ptr byval(%item) align 1 %a) nosync memory(read) {
; CHECK-LABEL: define internal i32 @outgoing_musttail(ptr byval(%item) align 1 %a)
  %local = alloca %item, align 1
  %value = load i32, ptr %a, align 1
  store i32 %value, ptr %local, align 1
  %result = musttail call i32 @tail_sink(ptr byval(%item) align 1 %local)
  ret i32 %result
}

define internal i32 @bundled(ptr byval(%item) align 1 %a) nosync memory(read) {
; CHECK-LABEL: define internal i32 @bundled(ptr byval(%item) align 1 %a)
  %value = load i32, ptr %a, align 1
  ret i32 %value
}
