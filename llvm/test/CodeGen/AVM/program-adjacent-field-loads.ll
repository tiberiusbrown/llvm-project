; RUN: llc -mtriple=avm -O2 -verify-machineinstrs < %s | FileCheck %s

@records = external addrspace(1) constant [256 x { ptr addrspace(1), ptr addrspace(1) }], align 1

; DAG reassociation folds the field offset into a second symbol address. Keep
; one dynamic record base and use the first 24-bit load's +3 writeback for the
; second field, even with a dependent load between the field accesses.
define i16 @record_fields(i8 %index) {
; CHECK-LABEL: record_fields:
; CHECK: ldi16 {{r[0-7]}}, %lo16(records)
; CHECK-NEXT: ldi8 {{r[0-7]}}, %hi8(records)
; CHECK: ldp24 {{q[0-3]}}, [{{q[0-3]}}+]
; CHECK-NOT: %lo16(records+3)
; CHECK-NOT: %hi8(records+3)
; CHECK: ldp8u
; CHECK: ldp24
; CHECK: ldp8u
; CHECK: ret
entry:
  %i = zext i8 %index to i24
  %a = getelementptr inbounds [256 x { ptr addrspace(1), ptr addrspace(1) }], ptr addrspace(1) @records, i24 0, i24 %i, i32 0
  %data = load ptr addrspace(1), ptr addrspace(1) %a, align 1
  %av = load i8, ptr addrspace(1) %data, align 1
  %b = getelementptr inbounds [256 x { ptr addrspace(1), ptr addrspace(1) }], ptr addrspace(1) @records, i24 0, i24 %i, i32 1
  %title = load ptr addrspace(1), ptr addrspace(1) %b, align 1
  %bv = load i8, ptr addrspace(1) %title, align 1
  %aw = zext i8 %av to i16
  %bw = zext i8 %bv to i16
  %sum = add i16 %aw, %bw
  ret i16 %sum
}

; Volatile accesses must not be paired through a new address dependency.
define i16 @volatile_fields(ptr addrspace(1) %base, i24 %offset) {
; CHECK-LABEL: volatile_fields:
; CHECK-NOT: ldp8u {{.*}}+
; CHECK: ldp8u
; CHECK-NOT: ldp8u {{.*}}+
; CHECK: ldp8u
; CHECK-NOT: ldp8u {{.*}}+
; CHECK: ret
entry:
  %a = getelementptr i8, ptr addrspace(1) %base, i24 %offset
  %av = load volatile i8, ptr addrspace(1) %a, align 1
  %boff = add i24 %offset, 1
  %b = getelementptr i8, ptr addrspace(1) %base, i24 %boff
  %bv = load volatile i8, ptr addrspace(1) %b, align 1
  %aw = zext i8 %av to i16
  %bw = zext i8 %bv to i16
  %sum = add i16 %aw, %bw
  ret i16 %sum
}

declare void @observe(i8)

; The higher-address load feeds a call before the lower-address load. Making
; the higher address depend on the lower load's writeback would create a DAG
; cycle through the call's chain.
define i16 @reverse_dependency(ptr addrspace(1) %base, i24 %offset) {
; CHECK-LABEL: reverse_dependency:
; CHECK-NOT: ldp8u {{.*}}+
; CHECK: ldp8u
; CHECK-NOT: ldp8u {{.*}}+
; CHECK: call
; CHECK: ldp8u
; CHECK-NOT: ldp8u {{.*}}+
; CHECK: ret
entry:
  %a = getelementptr i8, ptr addrspace(1) %base, i24 %offset
  %b = getelementptr i8, ptr addrspace(1) %a, i24 1
  %bv = load i8, ptr addrspace(1) %b, align 1
  call void @observe(i8 %bv)
  %av = load i8, ptr addrspace(1) %a, align 1
  %aw = zext i8 %av to i16
  %bw = zext i8 %bv to i16
  %sum = add i16 %aw, %bw
  ret i16 %sum
}
