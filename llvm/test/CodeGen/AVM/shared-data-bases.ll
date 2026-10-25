; RUN: llc -mtriple=avm -O2 -verify-machineinstrs %s -o - | FileCheck %s
; RUN: llc -mtriple=avm -O2 -stop-after=finalize-isel -verify-machineinstrs %s -o - | FileCheck %s --check-prefix=ISEL

; Local symbols allow DAG combining to fold each offset into the relocation.
@data = internal global [1024 x i8] zeroinitializer, align 1
@program = external addrspace(1) global [1024 x i8], align 1

define i16 @shared_bytes(i16 %index) {
; CHECK-LABEL: shared_bytes:
; ISEL-LABEL: name: shared_bytes
; ISEL: %[[BASE:[0-9]+]]:gpr16 = ADD16_PSEUDO
; ISEL: LOAD8U_PSEUDO {{(killed )?}}%[[BASE]]
; ISEL: LOAD8U_DISP_PSEUDO {{(killed )?}}%[[BASE]], 1
; ISEL: LOAD8U_DISP_PSEUDO {{(killed )?}}%[[BASE]], 15
; ISEL: LOAD8U_DISP_PSEUDO {{(killed )?}}%[[BASE]], 223
; ISEL-NOT: DATA_ADDR_PSEUDO
; ISEL: RET_PSEUDO
  %p0 = getelementptr i8, ptr getelementptr (i8, ptr @data, i16 0), i16 %index
  %v0 = load volatile i8, ptr %p0, align 1
  %w0 = zext i8 %v0 to i16
  %p1 = getelementptr i8, ptr getelementptr (i8, ptr @data, i16 1), i16 %index
  %v1 = load volatile i8, ptr %p1, align 1
  %w1 = zext i8 %v1 to i16
  %p2 = getelementptr i8, ptr getelementptr (i8, ptr @data, i16 15), i16 %index
  %v2 = load volatile i8, ptr %p2, align 1
  %w2 = zext i8 %v2 to i16
  %p3 = getelementptr i8, ptr getelementptr (i8, ptr @data, i16 223), i16 %index
  %v3 = load volatile i8, ptr %p3, align 1
  %w3 = zext i8 %v3 to i16
  %sum1 = add i16 %w0, %w1
  %sum2 = add i16 %sum1, %w2
  %sum3 = add i16 %sum2, %w3
  %result = add i16 %sum3, %index
  ret i16 %result
}

define i16 @unrolled_loop(i16 %start) {
; CHECK-LABEL: unrolled_loop:
; ISEL-LABEL: name: unrolled_loop
; ISEL: %[[BASE:[0-9]+]]:gpr16 = ADD16_PSEUDO
; ISEL: LOAD8U_PSEUDO {{(killed )?}}%[[BASE]]
; ISEL: LOAD8U_DISP_PSEUDO {{(killed )?}}%[[BASE]], 1
; ISEL: LOAD8U_DISP_PSEUDO {{(killed )?}}%[[BASE]], 2
; ISEL: LOAD8U_DISP_PSEUDO {{(killed )?}}%[[BASE]], 3
; ISEL: LOAD8U_DISP_PSEUDO {{(killed )?}}%[[BASE]], 4
; ISEL: LOAD8U_DISP_PSEUDO {{(killed )?}}%[[BASE]], 5
; ISEL: LOAD8U_DISP_PSEUDO {{(killed )?}}%[[BASE]], 6
; ISEL: LOAD8U_DISP_PSEUDO {{(killed )?}}%[[BASE]], 7
; ISEL: LOAD8U_DISP_PSEUDO {{(killed )?}}%[[BASE]], 8
; ISEL: LOAD8U_DISP_PSEUDO {{(killed )?}}%[[BASE]], 9
; ISEL: LOAD8U_DISP_PSEUDO {{(killed )?}}%[[BASE]], 10
; ISEL: LOAD8U_DISP_PSEUDO {{(killed )?}}%[[BASE]], 11
; ISEL: LOAD8U_DISP_PSEUDO {{(killed )?}}%[[BASE]], 12
; ISEL: LOAD8U_DISP_PSEUDO {{(killed )?}}%[[BASE]], 13
; ISEL: LOAD8U_DISP_PSEUDO {{(killed )?}}%[[BASE]], 14
; ISEL: LOAD8U_DISP_PSEUDO {{(killed )?}}%[[BASE]], 15
; ISEL-NOT: DATA_ADDR_PSEUDO
; ISEL: RET_PSEUDO
entry:
  br label %loop
loop:
  %index = phi i16 [%start, %entry], [%next, %loop]
  %total = phi i16 [0, %entry], [%result, %loop]
  %p0 = getelementptr i8, ptr getelementptr (i8, ptr @data, i16 0), i16 %index
  %v0 = load volatile i8, ptr %p0, align 1
  %w0 = zext i8 %v0 to i16
  %p1 = getelementptr i8, ptr getelementptr (i8, ptr @data, i16 1), i16 %index
  %v1 = load volatile i8, ptr %p1, align 1
  %w1 = zext i8 %v1 to i16
  %p2 = getelementptr i8, ptr getelementptr (i8, ptr @data, i16 2), i16 %index
  %v2 = load volatile i8, ptr %p2, align 1
  %w2 = zext i8 %v2 to i16
  %p3 = getelementptr i8, ptr getelementptr (i8, ptr @data, i16 3), i16 %index
  %v3 = load volatile i8, ptr %p3, align 1
  %w3 = zext i8 %v3 to i16
  %p4 = getelementptr i8, ptr getelementptr (i8, ptr @data, i16 4), i16 %index
  %v4 = load volatile i8, ptr %p4, align 1
  %w4 = zext i8 %v4 to i16
  %p5 = getelementptr i8, ptr getelementptr (i8, ptr @data, i16 5), i16 %index
  %v5 = load volatile i8, ptr %p5, align 1
  %w5 = zext i8 %v5 to i16
  %p6 = getelementptr i8, ptr getelementptr (i8, ptr @data, i16 6), i16 %index
  %v6 = load volatile i8, ptr %p6, align 1
  %w6 = zext i8 %v6 to i16
  %p7 = getelementptr i8, ptr getelementptr (i8, ptr @data, i16 7), i16 %index
  %v7 = load volatile i8, ptr %p7, align 1
  %w7 = zext i8 %v7 to i16
  %p8 = getelementptr i8, ptr getelementptr (i8, ptr @data, i16 8), i16 %index
  %v8 = load volatile i8, ptr %p8, align 1
  %w8 = zext i8 %v8 to i16
  %p9 = getelementptr i8, ptr getelementptr (i8, ptr @data, i16 9), i16 %index
  %v9 = load volatile i8, ptr %p9, align 1
  %w9 = zext i8 %v9 to i16
  %p10 = getelementptr i8, ptr getelementptr (i8, ptr @data, i16 10), i16 %index
  %v10 = load volatile i8, ptr %p10, align 1
  %w10 = zext i8 %v10 to i16
  %p11 = getelementptr i8, ptr getelementptr (i8, ptr @data, i16 11), i16 %index
  %v11 = load volatile i8, ptr %p11, align 1
  %w11 = zext i8 %v11 to i16
  %p12 = getelementptr i8, ptr getelementptr (i8, ptr @data, i16 12), i16 %index
  %v12 = load volatile i8, ptr %p12, align 1
  %w12 = zext i8 %v12 to i16
  %p13 = getelementptr i8, ptr getelementptr (i8, ptr @data, i16 13), i16 %index
  %v13 = load volatile i8, ptr %p13, align 1
  %w13 = zext i8 %v13 to i16
  %p14 = getelementptr i8, ptr getelementptr (i8, ptr @data, i16 14), i16 %index
  %v14 = load volatile i8, ptr %p14, align 1
  %w14 = zext i8 %v14 to i16
  %p15 = getelementptr i8, ptr getelementptr (i8, ptr @data, i16 15), i16 %index
  %v15 = load volatile i8, ptr %p15, align 1
  %w15 = zext i8 %v15 to i16
  %sum1 = add i16 %w0, %w1
  %sum2 = add i16 %sum1, %w2
  %sum3 = add i16 %sum2, %w3
  %sum4 = add i16 %sum3, %w4
  %sum5 = add i16 %sum4, %w5
  %sum6 = add i16 %sum5, %w6
  %sum7 = add i16 %sum6, %w7
  %sum8 = add i16 %sum7, %w8
  %sum9 = add i16 %sum8, %w9
  %sum10 = add i16 %sum9, %w10
  %sum11 = add i16 %sum10, %w11
  %sum12 = add i16 %sum11, %w12
  %sum13 = add i16 %sum12, %w13
  %sum14 = add i16 %sum13, %w14
  %sum15 = add i16 %sum14, %w15
  %result = add i16 %sum15, %total
  %next = add i16 %index, 1
  %done = icmp eq i16 %next, 64
  br i1 %done, label %exit, label %loop
exit:
  ret i16 %result
}

define i16 @shared_words(i16 %index) {
; CHECK-LABEL: shared_words:
; ISEL-LABEL: name: shared_words
; ISEL: %[[BASE:[0-9]+]]:gpr16 = ADD16_PSEUDO
; ISEL: LOAD16_PSEUDO {{(killed )?}}%[[BASE]]
; ISEL: LOAD16_DISP_PSEUDO {{(killed )?}}%[[BASE]], 2
; ISEL: LOAD16_DISP_PSEUDO {{(killed )?}}%[[BASE]], 14
; ISEL: LOAD16_DISP_PSEUDO {{(killed )?}}%[[BASE]], 30
; ISEL-NOT: DATA_ADDR_PSEUDO
; ISEL: RET_PSEUDO
  %p0 = getelementptr i8, ptr getelementptr (i8, ptr @data, i16 0), i16 %index
  %v0 = load volatile i16, ptr %p0, align 1
  %p1 = getelementptr i8, ptr getelementptr (i8, ptr @data, i16 2), i16 %index
  %v1 = load volatile i16, ptr %p1, align 1
  %p2 = getelementptr i8, ptr getelementptr (i8, ptr @data, i16 14), i16 %index
  %v2 = load volatile i16, ptr %p2, align 1
  %p3 = getelementptr i8, ptr getelementptr (i8, ptr @data, i16 30), i16 %index
  %v3 = load volatile i16, ptr %p3, align 1
  %sum1 = add i16 %v0, %v1
  %sum2 = add i16 %sum1, %v2
  %sum3 = add i16 %sum2, %v3
  %result = add i16 %sum3, %index
  ret i16 %result
}

define i16 @displacement_limits(i16 %index) {
; CHECK-LABEL: displacement_limits:
; ISEL-LABEL: name: displacement_limits
; ISEL: %[[BASE:[0-9]+]]:gpr16 = ADD16_PSEUDO
; ISEL: LOAD8U_DISP_PSEUDO {{(killed )?}}%[[BASE]], -32
; ISEL: LOAD8U_PSEUDO {{(killed )?}}%[[BASE]]
; ISEL: LOAD8U_DISP_PSEUDO {{(killed )?}}%[[BASE]], 223
; ISEL-NOT: DATA_ADDR_PSEUDO
; ISEL: RET_PSEUDO
  %p0 = getelementptr i8, ptr getelementptr (i8, ptr @data, i16 -32), i16 %index
  %v0 = load volatile i8, ptr %p0, align 1
  %w0 = zext i8 %v0 to i16
  %p1 = getelementptr i8, ptr getelementptr (i8, ptr @data, i16 0), i16 %index
  %v1 = load volatile i8, ptr %p1, align 1
  %w1 = zext i8 %v1 to i16
  %p2 = getelementptr i8, ptr getelementptr (i8, ptr @data, i16 223), i16 %index
  %v2 = load volatile i8, ptr %p2, align 1
  %w2 = zext i8 %v2 to i16
  %sum1 = add i16 %w0, %w1
  %sum2 = add i16 %sum1, %w2
  %result = add i16 %sum2, %index
  ret i16 %result
}

define i16 @outside_displacement(i16 %index) {
; CHECK-LABEL: outside_displacement:
; ISEL-LABEL: name: outside_displacement
; ISEL-NOT: DISP_PSEUDO
; ISEL: RET_PSEUDO
  %p0 = getelementptr i8, ptr getelementptr (i8, ptr @data, i16 0), i16 %index
  %v0 = load volatile i8, ptr %p0, align 1
  %w0 = zext i8 %v0 to i16
  %p1 = getelementptr i8, ptr getelementptr (i8, ptr @data, i16 256), i16 %index
  %v1 = load volatile i8, ptr %p1, align 1
  %w1 = zext i8 %v1 to i16
  %p2 = getelementptr i8, ptr getelementptr (i8, ptr @data, i16 512), i16 %index
  %v2 = load volatile i8, ptr %p2, align 1
  %w2 = zext i8 %v2 to i16
  %sum1 = add i16 %w0, %w1
  %sum2 = add i16 %sum1, %w2
  %result = add i16 %sum2, %index
  ret i16 %result
}

define i16 @wrapping_offsets(i16 %index) {
; CHECK-LABEL: wrapping_offsets:
; ISEL-LABEL: name: wrapping_offsets
; ISEL: %[[BASE:[0-9]+]]:gpr16 = ADD16_PSEUDO
; ISEL: LOAD8U_PSEUDO {{(killed )?}}%[[BASE]]
; ISEL: LOAD8U_DISP_PSEUDO {{(killed )?}}%[[BASE]], 1
; ISEL: LOAD8U_DISP_PSEUDO {{(killed )?}}%[[BASE]], 2
; ISEL-NOT: DATA_ADDR_PSEUDO
; ISEL: RET_PSEUDO
  %p0 = getelementptr i8, ptr getelementptr (i8, ptr @data, i16 -1), i16 %index
  %v0 = load volatile i8, ptr %p0, align 1
  %w0 = zext i8 %v0 to i16
  %p1 = getelementptr i8, ptr getelementptr (i8, ptr @data, i16 0), i16 %index
  %v1 = load volatile i8, ptr %p1, align 1
  %w1 = zext i8 %v1 to i16
  %p2 = getelementptr i8, ptr getelementptr (i8, ptr @data, i16 1), i16 %index
  %v2 = load volatile i8, ptr %p2, align 1
  %w2 = zext i8 %v2 to i16
  %sum1 = add i16 %w0, %w1
  %sum2 = add i16 %sum1, %w2
  %result = add i16 %sum2, %index
  ret i16 %result
}

define i16 @cheap_immediates(i16 %index) {
; CHECK-LABEL: cheap_immediates:
; ISEL-LABEL: name: cheap_immediates
; ISEL-NOT: DISP_PSEUDO
; ISEL: ADDIS8_PSEUDO %0, -40
; ISEL-NOT: DISP_PSEUDO
; ISEL: ADDIS8_PSEUDO %0, -39
; ISEL-NOT: DISP_PSEUDO
; ISEL: RET_PSEUDO
  %i0 = add i16 %index, -40
  %p0 = inttoptr i16 %i0 to ptr
  %v0 = load volatile i8, ptr %p0, align 1
  %w0 = zext i8 %v0 to i16
  %i1 = add i16 %index, -39
  %p1 = inttoptr i16 %i1 to ptr
  %v1 = load volatile i8, ptr %p1, align 1
  %w1 = zext i8 %v1 to i16
  %sum1 = add i16 %w0, %w1
  %result = add i16 %sum1, %index
  ret i16 %result
}

define i16 @already_displaced(i16 %index) {
; CHECK-LABEL: already_displaced:
; ISEL-LABEL: name: already_displaced
; ISEL-NOT: ADD16_PSEUDO
; ISEL: LOAD8U_DISP_PSEUDO %0, 1
; ISEL: LOAD8U_DISP_PSEUDO %0, 2
; ISEL: LOAD8U_DISP_PSEUDO %0, 15
  %i0 = add i16 %index, 1
  %p0 = inttoptr i16 %i0 to ptr
  %v0 = load volatile i8, ptr %p0, align 1
  %w0 = zext i8 %v0 to i16
  %i1 = add i16 %index, 2
  %p1 = inttoptr i16 %i1 to ptr
  %v1 = load volatile i8, ptr %p1, align 1
  %w1 = zext i8 %v1 to i16
  %i2 = add i16 %index, 15
  %p2 = inttoptr i16 %i2 to ptr
  %v2 = load volatile i8, ptr %p2, align 1
  %w2 = zext i8 %v2 to i16
  %sum1 = add i16 %w0, %w1
  %sum2 = add i16 %sum1, %w2
  %result = add i16 %sum2, %index
  ret i16 %result
}

define i16 @large_immediates(i16 %index) {
; CHECK-LABEL: large_immediates:
; ISEL-LABEL: name: large_immediates
; ISEL: %[[BASE:[0-9]+]]:gpr16 = ADD16_PSEUDO
; ISEL: LOAD8U_PSEUDO {{(killed )?}}%[[BASE]]
; ISEL: LOAD8U_DISP_PSEUDO {{(killed )?}}%[[BASE]], 1
; ISEL: LOAD8U_DISP_PSEUDO {{(killed )?}}%[[BASE]], 16
; ISEL-NOT: DATA_ADDR_PSEUDO
; ISEL: RET_PSEUDO
  %i0 = add i16 %index, 1024
  %p0 = inttoptr i16 %i0 to ptr
  %v0 = load volatile i8, ptr %p0, align 1
  %w0 = zext i8 %v0 to i16
  %i1 = add i16 %index, 1025
  %p1 = inttoptr i16 %i1 to ptr
  %v1 = load volatile i8, ptr %p1, align 1
  %w1 = zext i8 %v1 to i16
  %i2 = add i16 %index, 1040
  %p2 = inttoptr i16 %i2 to ptr
  %v2 = load volatile i8, ptr %p2, align 1
  %w2 = zext i8 %v2 to i16
  %sum1 = add i16 %w0, %w1
  %sum2 = add i16 %sum1, %w2
  %result = add i16 %sum2, %index
  ret i16 %result
}

define i16 @program_addresses(i16 %index) {
; CHECK-LABEL: program_addresses:
; ISEL-LABEL: name: program_addresses
; ISEL-NOT: LOAD8U_DISP_PSEUDO
; ISEL: PLOAD8U_PSEUDO
; ISEL-NOT: LOAD8U_DISP_PSEUDO
; ISEL: RET_PSEUDO
  %p0 = getelementptr i8, ptr addrspace(1) getelementptr (i8, ptr addrspace(1) @program, i16 0), i16 %index
  %v0 = load volatile i8, ptr addrspace(1) %p0, align 1
  %w0 = zext i8 %v0 to i16
  %p1 = getelementptr i8, ptr addrspace(1) getelementptr (i8, ptr addrspace(1) @program, i16 1), i16 %index
  %v1 = load volatile i8, ptr addrspace(1) %p1, align 1
  %w1 = zext i8 %v1 to i16
  %p2 = getelementptr i8, ptr addrspace(1) getelementptr (i8, ptr addrspace(1) @program, i16 15), i16 %index
  %v2 = load volatile i8, ptr addrspace(1) %p2, align 1
  %w2 = zext i8 %v2 to i16
  %sum1 = add i16 %w0, %w1
  %sum2 = add i16 %sum1, %w2
  %result = add i16 %sum2, %index
  ret i16 %result
}

define void @store_bytes(i16 %index, i8 %value) {
; CHECK-LABEL: store_bytes:
; ISEL-LABEL: name: store_bytes
; ISEL: %[[BASE:[0-9]+]]:gpr16 = ADD16_PSEUDO
; ISEL: STORE8_PSEUDO {{(killed )?}}%[[BASE]]
; ISEL: STORE8_DISP_PSEUDO {{(killed )?}}%[[BASE]], 1
; ISEL: STORE8_DISP_PSEUDO {{(killed )?}}%[[BASE]], 15
; ISEL: STORE8_DISP_PSEUDO {{(killed )?}}%[[BASE]], 223
; ISEL-NOT: DATA_ADDR_PSEUDO
; ISEL: RET_PSEUDO
  %p0 = getelementptr i8, ptr getelementptr (i8, ptr @data, i16 0), i16 %index
  store volatile i8 %value, ptr %p0, align 1
  %p1 = getelementptr i8, ptr getelementptr (i8, ptr @data, i16 1), i16 %index
  store volatile i8 %value, ptr %p1, align 1
  %p2 = getelementptr i8, ptr getelementptr (i8, ptr @data, i16 15), i16 %index
  store volatile i8 %value, ptr %p2, align 1
  %p3 = getelementptr i8, ptr getelementptr (i8, ptr @data, i16 223), i16 %index
  store volatile i8 %value, ptr %p3, align 1
  ret void
}

define void @store_words(i16 %index, i16 %value) {
; CHECK-LABEL: store_words:
; ISEL-LABEL: name: store_words
; ISEL: %[[BASE:[0-9]+]]:gpr16 = ADD16_PSEUDO
; ISEL: STORE16_PSEUDO {{(killed )?}}%[[BASE]]
; ISEL: STORE16_DISP_PSEUDO {{(killed )?}}%[[BASE]], 2
; ISEL: STORE16_DISP_PSEUDO {{(killed )?}}%[[BASE]], 14
; ISEL: STORE16_DISP_PSEUDO {{(killed )?}}%[[BASE]], 30
; ISEL-NOT: DATA_ADDR_PSEUDO
; ISEL: RET_PSEUDO
  %p0 = getelementptr i8, ptr getelementptr (i8, ptr @data, i16 0), i16 %index
  store volatile i16 %value, ptr %p0, align 1
  %p1 = getelementptr i8, ptr getelementptr (i8, ptr @data, i16 2), i16 %index
  store volatile i16 %value, ptr %p1, align 1
  %p2 = getelementptr i8, ptr getelementptr (i8, ptr @data, i16 14), i16 %index
  store volatile i16 %value, ptr %p2, align 1
  %p3 = getelementptr i8, ptr getelementptr (i8, ptr @data, i16 30), i16 %index
  store volatile i16 %value, ptr %p3, align 1
  ret void
}

; A wide field needs its own pointer and additional pair registers.
; Keep the original address strategy for this mixed-width group.
define i32 @mixed_width_fields(i16 %index) {
; CHECK-LABEL: mixed_width_fields:
; CHECK: ld32
; CHECK: ret
; ISEL-LABEL: name: mixed_width_fields
; ISEL-NOT: DISP_PSEUDO
; ISEL: LOAD32_PSEUDO
; ISEL-NOT: DISP_PSEUDO
; ISEL: RET_PSEUDO
  %a = getelementptr i8, ptr @data, i16 %index
  %b = getelementptr i8, ptr getelementptr (i8, ptr @data, i16 4), i16 %index
  %c = getelementptr i8, ptr getelementptr (i8, ptr @data, i16 5), i16 %index
  %d = getelementptr i8, ptr getelementptr (i8, ptr @data, i16 7), i16 %index
  %va = load volatile i32, ptr %a, align 1
  %vb = load volatile i8, ptr %b, align 1
  %vc = load volatile i16, ptr %c, align 1
  %vd = load volatile i8, ptr %d, align 1
  %wb = zext i8 %vb to i32
  %wc = zext i16 %vc to i32
  %wd = zext i8 %vd to i32
  %s1 = add i32 %va, %wb
  %s2 = add i32 %s1, %wc
  %s3 = add i32 %s2, %wd
  ret i32 %s3
}

; Widening multiplication replaces SEXTLOAD nodes during selection. The
; replacement must keep the chosen base, volatility and memory provenance.
define i16 @signed_products(i16 %index, i8 signext %coefficient, i8 signext %coefficient2, i8 signext %coefficient3) {
; CHECK-LABEL: signed_products:
; CHECK-NOT: sext8
; CHECK: muls8.w
; CHECK: muls8.w
; CHECK: muls8.w
; CHECK-NOT: sext8
; CHECK: ret
; ISEL-LABEL: name: signed_products
; ISEL: %[[BASE:[0-9]+]]:gpr16 = ADD16_PSEUDO
; ISEL: LOAD8U_PSEUDO %[[BASE]] :: (volatile load (s8) from %ir.a)
; ISEL: LOAD8U_DISP_PSEUDO %[[BASE]], 1 :: (volatile load (s8) from %ir.b)
; ISEL: LOAD8U_DISP_PSEUDO {{(killed )?}}%[[BASE]], 15 :: (volatile load (s8) from %ir.c)
  %a = getelementptr i8, ptr @data, i16 %index
  %b = getelementptr i8, ptr getelementptr (i8, ptr @data, i16 1), i16 %index
  %c = getelementptr i8, ptr getelementptr (i8, ptr @data, i16 15), i16 %index
  %va = load volatile i8, ptr %a, align 1
  %vb = load volatile i8, ptr %b, align 1
  %vc = load volatile i8, ptr %c, align 1
  %wa = sext i8 %va to i16
  %wb = sext i8 %vb to i16
  %wc = sext i8 %vc to i16
  %coef = sext i8 %coefficient to i16
  %coef2 = sext i8 %coefficient2 to i16
  %coef3 = sext i8 %coefficient3 to i16
  %pa = mul i16 %wa, %coef
  %pb = mul i16 %wb, %coef2
  %pc = mul i16 %wc, %coef3
  %s1 = add i16 %pa, %pb
  %s2 = add i16 %s1, %pc
  ret i16 %s2
}

; Address arithmetic with non-memory users remains necessary, so it cannot
; pay for slower displaced loads.
define i16 @address_values_remain_live(i16 %index) {
; CHECK-LABEL: address_values_remain_live:
; CHECK: ret
; ISEL-LABEL: name: address_values_remain_live
; ISEL-NOT: DISP_PSEUDO
; ISEL-DAG: DATA_ADDR_PSEUDO @data{{$}}
; ISEL-DAG: DATA_ADDR_PSEUDO @data + 1
; ISEL-NOT: DISP_PSEUDO
; ISEL: RET_PSEUDO
  %a = getelementptr i8, ptr @data, i16 %index
  %b = getelementptr i8, ptr getelementptr (i8, ptr @data, i16 1), i16 %index
  %va = load volatile i8, ptr %a, align 1
  %vb = load volatile i8, ptr %b, align 1
  %ai = ptrtoint ptr %a to i16
  %bi = ptrtoint ptr %b to i16
  %addresses = xor i16 %ai, %bi
  %wa = zext i8 %va to i16
  %wb = zext i8 %vb to i16
  %sum = add i16 %wa, %wb
  %result = xor i16 %addresses, %sum
  ret i16 %result
}
