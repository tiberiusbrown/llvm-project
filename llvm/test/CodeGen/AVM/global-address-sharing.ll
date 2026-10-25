; RUN: llc -mtriple=avm -O2 -verify-machineinstrs %s -o - | FileCheck %s
; RUN: llc -mtriple=avm -O2 -stop-after=finalize-isel -verify-machineinstrs %s -o - | FileCheck %s --check-prefix=ISEL
; RUN: llc -mtriple=avm -O2 -enable-global-merge=false -verify-machineinstrs %s -o - | FileCheck %s --check-prefix=SEPARATE
; RUN: llc -mtriple=avm -O0 -verify-machineinstrs %s -o - | FileCheck %s --check-prefix=SEPARATE

; Separate local arrays need a compiler-established layout before their
; addresses can share a dynamic base. Volatile loads must retain their order
; and memory operands through both merging and address-mode selection.
@a = internal global [32 x i8] zeroinitializer, align 1
@b = internal global [32 x i8] zeroinitializer, align 1
@c = internal global [32 x i8] zeroinitializer, align 1
@d = internal global [32 x i8] zeroinitializer, align 1
@public_a = global [32 x i8] zeroinitializer, align 1
@public_b = global [32 x i8] zeroinitializer, align 1

define i16 @shared_arrays(i16 %index) {
; CHECK-LABEL: shared_arrays:
; CHECK: ld8u {{r[0-7]}}, [{{r[0-7]}}]
; CHECK: ld8u {{r[0-7]}}, [{{r[0-7]}}+32]
; CHECK: ld8u {{r[0-7]}}, [{{r[0-7]}}+64]
; CHECK: ld8u {{r[0-7]}}, [{{r[0-7]}}+96]
; ISEL-LABEL: name: shared_arrays
; ISEL: %[[SYMBOL:[0-9]+]]:gpr16 = DATA_ADDR_PSEUDO @_MergedGlobals
; ISEL: %[[BASE:[0-9]+]]:gpr16 = ADD16_PSEUDO %0, killed %[[SYMBOL]]
; ISEL: LOAD8U_PSEUDO %[[BASE]] :: (volatile load (s8) from %ir.pa)
; ISEL: LOAD8U_DISP_PSEUDO %[[BASE]], 32 :: (volatile load (s8) from %ir.pb)
; ISEL: LOAD8U_DISP_PSEUDO %[[BASE]], 64 :: (volatile load (s8) from %ir.pc)
; ISEL: LOAD8U_DISP_PSEUDO {{(killed )?}}%[[BASE]], 96 :: (volatile load (s8) from %ir.pd)
; ISEL-NOT: DATA_ADDR_PSEUDO
; ISEL: RET_PSEUDO
; SEPARATE-LABEL: shared_arrays:
; SEPARATE-NOT: _MergedGlobals
; SEPARATE-NOT: ld8u {{.*}}+32]
; SEPARATE-NOT: ld8u {{.*}}+64]
; SEPARATE-NOT: ld8u {{.*}}+96]
; SEPARATE: ret
  %pa = getelementptr [32 x i8], ptr @a, i16 0, i16 %index
  %va = load volatile i8, ptr %pa, align 1
  %pb = getelementptr [32 x i8], ptr @b, i16 0, i16 %index
  %vb = load volatile i8, ptr %pb, align 1
  %pc = getelementptr [32 x i8], ptr @c, i16 0, i16 %index
  %vc = load volatile i8, ptr %pc, align 1
  %pd = getelementptr [32 x i8], ptr @d, i16 0, i16 %index
  %vd = load volatile i8, ptr %pd, align 1
  %wa = sext i8 %va to i16
  %wb = sext i8 %vb to i16
  %wc = zext i8 %vc to i16
  %wd = zext i8 %vd to i16
  %ab = add i16 %wa, %wb
  %cd = add i16 %wc, %wd
  %result = add i16 %ab, %cd
  ret i16 %result
}

; Establishing a common object before LSR would change the pointer-IV policy.
; Merge after loop preparation: keep the scalar index and share its base in
; the loop's DAG instead of creating additional loop-carried pointers.
define i16 @shared_loop() {
; ISEL-LABEL: name: shared_loop
; ISEL: bb.1.loop:
; ISEL: DATA_ADDR_PSEUDO @_MergedGlobals
; ISEL: %[[LOOPBASE:[0-9]+]]:gpr16 = ADD16_PSEUDO
; ISEL: LOAD8U_PSEUDO %[[LOOPBASE]]
; ISEL: LOAD8U_DISP_PSEUDO %[[LOOPBASE]], 32
; ISEL: LOAD8U_DISP_PSEUDO %[[LOOPBASE]], 64
; ISEL: LOAD8U_DISP_PSEUDO {{(killed )?}}%[[LOOPBASE]], 96
; ISEL-NOT: DATA_ADDR_PSEUDO
; ISEL: RET_PSEUDO
entry:
  br label %loop
loop:
  %index = phi i16 [0, %entry], [%next, %loop]
  %total = phi i16 [0, %entry], [%sum, %loop]
  %pa = getelementptr [32 x i8], ptr @a, i16 0, i16 %index
  %va = load volatile i8, ptr %pa, align 1
  %pb = getelementptr [32 x i8], ptr @b, i16 0, i16 %index
  %vb = load volatile i8, ptr %pb, align 1
  %pc = getelementptr [32 x i8], ptr @c, i16 0, i16 %index
  %vc = load volatile i8, ptr %pc, align 1
  %pd = getelementptr [32 x i8], ptr @d, i16 0, i16 %index
  %vd = load volatile i8, ptr %pd, align 1
  %wa = zext i8 %va to i16
  %wb = zext i8 %vb to i16
  %wc = zext i8 %vc to i16
  %wd = zext i8 %vd to i16
  %ab = add i16 %wa, %wb
  %cd = add i16 %wc, %wd
  %abcd = add i16 %ab, %cd
  %sum = add i16 %total, %abcd
  %next = add i16 %index, 1
  %done = icmp eq i16 %next, 32
  br i1 %done, label %exit, label %loop
exit:
  ret i16 %sum
}

; The extra pointer must pay for a possible spill when a live-in register pair
; competes with the index and loaded values. Merging still establishes a layout,
; but independent addresses are cheaper after the pressure allowance.
define i32 @pair_pressure(i16 %index, i32 %total) {
; ISEL-LABEL: name: pair_pressure
; ISEL-NOT: DISP_PSEUDO
; ISEL: LOAD16_PSEUDO
; ISEL-NOT: DISP_PSEUDO
; ISEL: LOAD16_PSEUDO
; ISEL-NOT: DISP_PSEUDO
; ISEL: LOAD16_PSEUDO
; ISEL-NOT: DISP_PSEUDO
; ISEL: LOAD16_PSEUDO
; ISEL-NOT: DISP_PSEUDO
; ISEL: RET_PSEUDO
  %pa = getelementptr i8, ptr @a, i16 %index
  %va = load volatile i16, ptr %pa, align 1
  %pb = getelementptr i8, ptr @b, i16 %index
  %vb = load volatile i16, ptr %pb, align 1
  %pc = getelementptr i8, ptr @c, i16 %index
  %vc = load volatile i16, ptr %pc, align 1
  %pd = getelementptr i8, ptr @d, i16 %index
  %vd = load volatile i16, ptr %pd, align 1
  %uq = udiv i16 %va, %vb
  %ur = urem i16 %va, %vb
  %sq = sdiv i16 %vc, %vd
  %sr = srem i16 %vc, %vd
  %wuq = zext i16 %uq to i32
  %wur = zext i16 %ur to i32
  %wsq = zext i16 %sq to i32
  %wsr = zext i16 %sr to i32
  %s1 = add i32 %total, %wuq
  %s2 = xor i32 %s1, %wur
  %s3 = add i32 %s2, %wsq
  %result = xor i32 %s3, %wsr
  ret i32 %result
}

; A pair input alone does not imply a spill. Two loads still have enough
; allocation headroom to share a base, as in mixed-width arithmetic loops.
define i32 @pair_small_group(i16 %index, i32 %total) {
; ISEL-LABEL: name: pair_small_group
; ISEL: %[[SMALLBASE:[0-9]+]]:gpr16 = ADD16_PSEUDO
; ISEL: LOAD8U_PSEUDO %[[SMALLBASE]]
; ISEL: LOAD8U_DISP_PSEUDO {{(killed )?}}%[[SMALLBASE]], 32
; ISEL: RET_PSEUDO
  %pa = getelementptr [32 x i8], ptr @a, i16 0, i16 %index
  %va = load volatile i8, ptr %pa, align 1
  %pb = getelementptr [32 x i8], ptr @b, i16 0, i16 %index
  %vb = load volatile i8, ptr %pb, align 1
  %wa = zext i8 %va to i32
  %wb = zext i8 %vb to i32
  %sum = add i32 %total, %wa
  %result = add i32 %sum, %wb
  ret i32 %result
}

; External definitions retain independently linkable storage and addresses.
define i16 @public_arrays(i16 %index) {
; ISEL-LABEL: name: public_arrays
; ISEL-DAG: DATA_ADDR_PSEUDO @public_a
; ISEL-DAG: DATA_ADDR_PSEUDO @public_b
; ISEL-NOT: DISP_PSEUDO
; ISEL: RET_PSEUDO
  %pa = getelementptr [32 x i8], ptr @public_a, i16 0, i16 %index
  %va = load volatile i8, ptr %pa, align 1
  %pb = getelementptr [32 x i8], ptr @public_b, i16 0, i16 %index
  %vb = load volatile i8, ptr %pb, align 1
  %wa = zext i8 %va to i16
  %wb = zext i8 %vb to i16
  %result = add i16 %wa, %wb
  ret i16 %result
}
