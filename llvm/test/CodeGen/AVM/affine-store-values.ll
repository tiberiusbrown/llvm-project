; RUN: llc -mtriple=avm -O2 -verify-machineinstrs %s -o - | FileCheck %s
; RUN: llc -mtriple=avm -O2 -stop-after=finalize-isel -verify-machineinstrs %s -o - | FileCheck %s --check-prefix=ISEL

declare void @consume(ptr)

; Promoted byte arithmetic must wrap at 256, including the 117/-126 boundary.
; Keep the full seed alive and check that every update reuses the store register.
define i16 @bytes(i16 %seed) {
; CHECK-LABEL: bytes:
; CHECK: addi.s8 [[R:r[0-7]]], -87
; CHECK-NEXT: stsp8 [sp+6], [[R]]
; CHECK-NEXT: addi.s8 [[R]], -13
; CHECK-NEXT: stsp8 [sp+5], [[R]]
; CHECK-NEXT: addi.s8 [[R]], -13
; CHECK-NEXT: stsp8 [sp+4], [[R]]
; CHECK-NEXT: addi.s8 [[R]], -13
; CHECK-NEXT: stsp8 [sp+3], [[R]]
; CHECK-NEXT: addi.s8 [[R]], -13
; CHECK-NEXT: stsp8 [sp+2], [[R]]
; CHECK-NEXT: addi.s8 [[R]], -13
; CHECK-NEXT: stsp8 [sp+1], [[R]]
; CHECK-NEXT: stsp8 [sp+0],
; ISEL-LABEL: name: bytes
; ISEL: %[[A:[0-9]+]]:gpr16 = ADDIS8_PSEUDO %{{[0-9]+}}, -87
; ISEL-NEXT: STACK_STORE8_PSEUDO %stack.0.array, 6, %[[A]],
; ISEL-NEXT: %[[B:[0-9]+]]:gpr16 = ADDIS8_PSEUDO %[[A]], -13
; ISEL-NEXT: STACK_STORE8_PSEUDO %stack.0.array, 5, %[[B]],
; ISEL-NEXT: %[[C:[0-9]+]]:gpr16 = ADDIS8_PSEUDO %[[B]], -13
  %array = alloca [7 x i8], align 1
  %byte = trunc i16 %seed to i8
  store i8 %byte, ptr %array
  %v1 = add i8 %byte, 104
  %p1 = getelementptr i8, ptr %array, i16 1
  store i8 %v1, ptr %p1
  %v2 = add i8 %byte, 117
  %p2 = getelementptr i8, ptr %array, i16 2
  store i8 %v2, ptr %p2
  %v3 = add i8 %byte, -126
  %p3 = getelementptr i8, ptr %array, i16 3
  store i8 %v3, ptr %p3
  %v4 = add i8 %byte, -113
  %p4 = getelementptr i8, ptr %array, i16 4
  store i8 %v4, ptr %p4
  %v5 = add i8 %byte, -100
  %p5 = getelementptr i8, ptr %array, i16 5
  store i8 %v5, ptr %p5
  %v6 = add i8 %byte, -87
  %p6 = getelementptr i8, ptr %array, i16 6
  store i8 %v6, ptr %p6
  call void @consume(ptr %array)
  ret i16 %seed
}

; Word deltas wrap at 65536. General RAM stores use the same recurrence.
define void @words(ptr %out, i16 %seed) {
; CHECK-LABEL: words:
; CHECK: st16{{.*}}[[W:r[0-7]]]
; CHECK-NEXT: addi.s8 [[W]], -7
; CHECK: st16{{.*}}[[W]]
; CHECK-NEXT: addi.s8 [[W]], -7
; CHECK: st16{{.*}}[[W]]
; CHECK-NEXT: addi.s8 [[W]], -7
; ISEL-LABEL: name: words
; ISEL: %[[W0:[0-9]+]]:gpr16 = ADD16_PSEUDO
; ISEL: STORE16{{.*}} %[[W0]]
; ISEL: %[[W1:[0-9]+]]:gpr16 = ADDIS8_PSEUDO %[[W0]], -7
; ISEL: STORE16{{.*}} %[[W1]]
; ISEL: %[[W2:[0-9]+]]:gpr16 = ADDIS8_PSEUDO %[[W1]], -7
  %v0 = add i16 %seed, 32760
  store i16 %v0, ptr %out, align 1
  %v1 = add i16 %seed, 32767
  %p1 = getelementptr i16, ptr %out, i16 1
  store i16 %v1, ptr %p1, align 1
  %v2 = add i16 %seed, -32762
  %p2 = getelementptr i16, ptr %out, i16 2
  store i16 %v2, ptr %p2, align 1
  %v3 = add i16 %seed, -32755
  %p3 = getelementptr i16, ptr %out, i16 3
  store i16 %v3, ptr %p3, align 1
  ret void
}

; No constant-stride or contiguous-array requirement: +1 selects inc.
define void @irregular(ptr %out, i16 %seed) {
; ISEL-LABEL: name: irregular
; ISEL: %[[I0:[0-9]+]]:gpr16 = ADDIS8_PSEUDO %{{[0-9]+}}, 40
; ISEL: %[[I1:[0-9]+]]:gpr16 = INC16_PSEUDO %[[I0]]
; ISEL: %[[I2:[0-9]+]]:gpr16 = ADDIS8_PSEUDO %[[I1]], 13
; ISEL: %{{[0-9]+}}:gpr16 = ADDIS8_PSEUDO %[[I2]], -27
  %v0 = add i16 %seed, 27
  store i16 %v0, ptr %out, align 1
  %v1 = add i16 %seed, 54
  %p1 = getelementptr i16, ptr %out, i16 3
  store i16 %v1, ptr %p1, align 1
  %v2 = add i16 %seed, 41
  %p2 = getelementptr i16, ptr %out, i16 7
  store i16 %v2, ptr %p2, align 1
  %v3 = add i16 %seed, 40
  %p3 = getelementptr i16, ptr %out, i16 9
  store i16 %v3, ptr %p3, align 1
  ret void
}

define void @decrement(ptr %out, i16 %seed) {
; ISEL-LABEL: name: decrement
; ISEL: %[[D:[0-9]+]]:gpr16 = ADDIS8_PSEUDO %{{[0-9]+}}, 41
; ISEL: DEC16_PSEUDO %[[D]]
  %v0 = add i16 %seed, 40
  store i16 %v0, ptr %out, align 1
  %v1 = add i16 %seed, 41
  %p1 = getelementptr i16, ptr %out, i16 1
  store i16 %v1, ptr %p1, align 1
  ret void
}

; A byte value also used at full width must retain its exact upper bits.
define i16 @shared_value(ptr %out, i16 %seed) {
; ISEL-LABEL: name: shared_value
; ISEL: %[[SEED:[0-9]+]]:gpr16 = COPY $r5
; ISEL: ADDIS8_PSEUDO %[[SEED]], 117
; ISEL: ADDIS8_PSEUDO %[[SEED]], -126
  %v0 = add i16 %seed, 117
  %b0 = trunc i16 %v0 to i8
  store i8 %b0, ptr %out
  %v1 = add i16 %seed, -126
  %b1 = trunc i16 %v1 to i8
  %p1 = getelementptr i8, ptr %out, i16 1
  store i8 %b1, ptr %p1
  ret i16 %v1
}

; Memory dependencies, volatile/atomic accesses, and expensive word deltas
; must not be turned into a recurrence.
define void @ordered(ptr %out, i16 %seed) {
; ISEL-LABEL: name: ordered
; ISEL: %[[S:[0-9]+]]:gpr16 = COPY $r5
; ISEL: ADDIS8_PSEUDO %[[S]], 13
; ISEL: ADDIS8_PSEUDO %[[S]], 26
; ISEL: ADDIS8_PSEUDO %[[S]], 39
  %v0 = add i16 %seed, 13
  store volatile i16 %v0, ptr %out, align 1
  %v1 = add i16 %seed, 26
  store volatile i16 %v1, ptr %out, align 1
  %v2 = add i16 %seed, 39
  store atomic i16 %v2, ptr %out unordered, align 1
  ret void
}

define void @large_delta(ptr %out, i16 %seed) {
; ISEL-LABEL: name: large_delta
; ISEL: %[[SEED:[0-9]+]]:gpr16 = COPY $r5
; ISEL: ADD16_PSEUDO %[[SEED]],
; ISEL: ADD16_PSEUDO %[[SEED]],
  %v0 = add i16 %seed, 300
  store i16 %v0, ptr %out, align 1
  %v1 = add i16 %seed, 600
  %p1 = getelementptr i16, ptr %out, i16 1
  store i16 %v1, ptr %p1, align 1
  ret void
}

define void @separated(ptr %out, i16 %seed) {
; ISEL-LABEL: name: separated
; ISEL: %[[SEED:[0-9]+]]:gpr16 = COPY $r5
; ISEL: ADDIS8_PSEUDO %[[SEED]], 13
; ISEL: CALL_DIRECT_PSEUDO @consume
; ISEL: ADDIS8_PSEUDO %[[SEED]], 26
  %v0 = add i16 %seed, 13
  store i16 %v0, ptr %out, align 1
  call void @consume(ptr %out)
  %v1 = add i16 %seed, 26
  %p1 = getelementptr i16, ptr %out, i16 1
  store i16 %v1, ptr %p1, align 1
  ret void
}

declare void @arguments(i16, i16, i16, i16, i16, i16, i16, i16, i32, i32)

; Outgoing argument stores must remain independently schedulable alongside
; register arguments. A recurrence here can spill unrelated live pairs.
define void @outgoing_arguments(i16 %seed, i32 %acc) {
; ISEL-LABEL: name: outgoing_arguments
; ISEL: %[[SEED:[0-9]+]]:gpr16 = COPY $r4
; ISEL-DAG: ADDIS8_PSEUDO %[[SEED]], 4
; ISEL-DAG: ADDIS8_PSEUDO %[[SEED]], 5
; ISEL-DAG: ADDIS8_PSEUDO %[[SEED]], 6
; ISEL-DAG: ADDIS8_PSEUDO %[[SEED]], 7
  %v1 = add i16 %seed, 1
  %v2 = add i16 %seed, 2
  %v3 = add i16 %seed, 3
  %v4 = add i16 %seed, 4
  %v5 = add i16 %seed, 5
  %v6 = add i16 %seed, 6
  %v7 = add i16 %seed, 7
  %wide = zext i16 %seed to i32
  %other = xor i32 %wide, 610839776
  call void @arguments(i16 %seed, i16 %v1, i16 %v2, i16 %v3,
                       i16 %v4, i16 %v5, i16 %v6, i16 %v7,
                       i32 %acc, i32 %other)
  ret void
}
