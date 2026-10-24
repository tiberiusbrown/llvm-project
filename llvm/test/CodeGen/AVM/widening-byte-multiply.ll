; RUN: llc -mtriple=avm -O2 -verify-machineinstrs < %s | FileCheck %s
; RUN: llc -mtriple=avm -O2 -stop-after=finalize-isel -verify-machineinstrs < %s | FileCheck %s --check-prefix=ISEL

define i16 @multiply_known_unsigned_bytes(i8 zeroext %a, i8 zeroext %b) {
; CHECK-LABEL: multiply_known_unsigned_bytes:
; CHECK:       mulu8.w
; CHECK-NOT:   mul16
entry:
  %a16 = zext i8 %a to i16
  %b16 = zext i8 %b to i16
  %combined = or i16 %a16, %b16
  %result = mul i16 %combined, %combined
  ret i16 %result
}

define i16 @multiply_promoted_narrow_induction(i16 %start) {
; CHECK-LABEL: multiply_promoted_narrow_induction:
; CHECK:       mulu8.w
; CHECK-NOT:   mul16
entry:
  br label %loop

loop:
  %value = phi i16 [ %start, %entry ], [ %next, %loop ]
  %result = mul nuw i16 %value, %value
  %next = add nuw i16 %value, 1
  %done = icmp eq i16 %next, 30
  br i1 %done, label %exit, label %loop

exit:
  ret i16 %result
}

; ABI assertions and live-out assertions describe the byte even though its
; register carrier is a word. No explicit extension is needed by the multiply.
define i16 @signed_arguments(i8 signext %a, i8 signext %b) {
; CHECK-LABEL: signed_arguments:
; CHECK-NOT: sext8
; CHECK: muls8.w
; CHECK-NOT: sext8
; CHECK: ret
; ISEL-LABEL: name: signed_arguments
; ISEL-NOT: SEXT8_PSEUDO
; ISEL: MULS8W_PSEUDO
; ISEL-NOT: SEXT8_PSEUDO
; ISEL: RET_PSEUDO
  %a16 = sext i8 %a to i16
  %b16 = sext i8 %b to i16
  %product = mul i16 %a16, %b16
  ret i16 %product
}

define i16 @mixed_arguments(i8 signext %a, i8 zeroext %b) {
; CHECK-LABEL: mixed_arguments:
; CHECK-NOT: sext8
; CHECK: mulsu8.w
; CHECK: ret
  %a16 = sext i8 %a to i16
  %b16 = zext i8 %b to i16
  %product = mul i16 %a16, %b16
  ret i16 %product
}

define i16 @mixed_arguments_reversed(i8 zeroext %a, i8 signext %b) {
; CHECK-LABEL: mixed_arguments_reversed:
; CHECK-NOT: sext8
; CHECK: mulsu8.w
; CHECK: ret
  %a16 = zext i8 %a to i16
  %b16 = sext i8 %b to i16
  %product = mul i16 %a16, %b16
  ret i16 %product
}

; These extensions have been folded into SEXTLOADs. Selecting a byte multiply
; must still avoid extending either operand, without duplicating volatile loads.
define i16 @signed_loads(ptr %a, ptr %b) {
; CHECK-LABEL: signed_loads:
; CHECK-NOT: sext8
; CHECK: muls8.w
; CHECK-NOT: sext8
; CHECK: ret
; ISEL-LABEL: name: signed_loads
; ISEL-NOT: SEXT8_PSEUDO
; ISEL: LOAD8U_PSEUDO
; ISEL-NOT: SEXT8_PSEUDO
; ISEL: LOAD8U_PSEUDO
; ISEL-NOT: LOAD8U_PSEUDO
; ISEL-NOT: SEXT8_PSEUDO
; ISEL: MULS8W_PSEUDO
; ISEL-NOT: LOAD8U_PSEUDO
; ISEL-NOT: SEXT8_PSEUDO
; ISEL: RET_PSEUDO
  %a8 = load volatile i8, ptr %a
  %b8 = load volatile i8, ptr %b
  %a16 = sext i8 %a8 to i16
  %b16 = sext i8 %b8 to i16
  %product = mul i16 %a16, %b16
  ret i16 %product
}

define i16 @mixed_program_loads(ptr addrspace(1) %a, ptr addrspace(1) %b) {
; CHECK-LABEL: mixed_program_loads:
; CHECK-NOT: sext8
; CHECK: mulsu8.w
; CHECK-NOT: sext8
; CHECK: ret
; ISEL-LABEL: name: mixed_program_loads
; ISEL-NOT: PLOAD8S_PSEUDO
; ISEL: PLOAD8U_PSEUDO
; ISEL: PLOAD8U_PSEUDO
; ISEL-NOT: SEXT8_PSEUDO
; ISEL: MULSU8W_PSEUDO
; ISEL: RET_PSEUDO
  %a8 = load volatile i8, ptr addrspace(1) %a
  %b8 = load volatile i8, ptr addrspace(1) %b
  %a16 = sext i8 %a8 to i16
  %b16 = zext i8 %b8 to i16
  %product = mul i16 %b16, %a16
  ret i16 %product
}

; Both value uses can belong to the same multiply, with just one memory read.
define i16 @signed_load_square(ptr %a) {
; CHECK-LABEL: signed_load_square:
; CHECK-NOT: sext8
; CHECK: muls8.w
; CHECK-NOT: sext8
; CHECK: ret
; ISEL-LABEL: name: signed_load_square
; ISEL: LOAD8U_PSEUDO
; ISEL-NOT: LOAD8U_PSEUDO
; ISEL-NOT: SEXT8_PSEUDO
; ISEL: MULS8W_PSEUDO
; ISEL: RET_PSEUDO
  %byte = load volatile i8, ptr %a
  %wide = sext i8 %byte to i16
  %product = mul i16 %wide, %wide
  ret i16 %product
}

; Keep the extension when another user needs the signed full word.
define i16 @shared_signed_load(ptr %a, i8 signext %b, ptr %out) {
; CHECK-LABEL: shared_signed_load:
; CHECK: sext8
; CHECK: muls8.w
; CHECK: ret
; ISEL-LABEL: name: shared_signed_load
; ISEL: LOAD8U_PSEUDO
; ISEL: SEXT8_PSEUDO
; ISEL: MULS8W_PSEUDO
; ISEL: RET_PSEUDO
  %a8 = load volatile i8, ptr %a
  %a16 = sext i8 %a8 to i16
  %b16 = sext i8 %b to i16
  store volatile i16 %a16, ptr %out
  %product = mul i16 %a16, %b16
  ret i16 %product
}

; FIR hoists signed coefficients out of the loop. The multiply sees an
; AssertSext on a CopyFromReg instead of the original extension or load.
define i16 @hoisted_signed_coefficient(ptr %a, ptr %b, i16 %count) {
; CHECK-LABEL: hoisted_signed_coefficient:
; CHECK: sext8
; CHECK: .LBB
; CHECK-NOT: sext8
; CHECK: muls8.w
; CHECK-NOT: sext8
; CHECK: ret
; ISEL-LABEL: name: hoisted_signed_coefficient
; ISEL: SEXT8_PSEUDO
; ISEL: bb.1.loop:
; ISEL-NOT: SEXT8_PSEUDO
; ISEL: MULS8W_PSEUDO
; ISEL-NOT: SEXT8_PSEUDO
; ISEL: RET_PSEUDO
entry:
  %coefficient = load volatile i8, ptr %b
  %wide = sext i8 %coefficient to i16
  br label %loop
loop:
  %index = phi i16 [0, %entry], [%next, %loop]
  %sum = phi i16 [0, %entry], [%total, %loop]
  %address = getelementptr i8, ptr %a, i16 %index
  %sample = load volatile i8, ptr %address
  %sample16 = sext i8 %sample to i16
  %product = mul i16 %wide, %sample16
  %total = add i16 %sum, %product
  %next = add i16 %index, 1
  %done = icmp eq i16 %next, %count
  br i1 %done, label %exit, label %loop
exit:
  ret i16 %total
}

; Signed-byte range proofs need not be literal eight-bit extensions.
define i16 @signed_nibbles(i4 signext %a, i4 signext %b) {
; CHECK-LABEL: signed_nibbles:
; CHECK: muls8.w
; CHECK: ret
  %a16 = sext i4 %a to i16
  %b16 = sext i4 %b to i16
  %product = mul i16 %a16, %b16
  ret i16 %product
}

define i16 @signed_ranges(i16 %a, i16 %b) {
; CHECK-LABEL: signed_ranges:
; CHECK: muls8.w
; CHECK: ret
  %a16 = ashr i16 %a, 8
  %b16 = ashr i16 %b, 9
  %product = mul i16 %a16, %b16
  ret i16 %product
}

; Unlike AssertSext, a narrow SIGN_EXTEND_INREG changes bits inside the low
; byte. Those bits must be extended before a signed byte multiply consumes it.
define i16 @signed_low_nibbles(i16 %a, i16 %b) {
; CHECK-LABEL: signed_low_nibbles:
; CHECK: lsl16i
; CHECK: asr16i
; CHECK: lsl16i
; CHECK: asr16i
; CHECK: muls8.w
; CHECK: ret
  %a4 = trunc i16 %a to i4
  %b4 = trunc i16 %b to i4
  %a16 = sext i4 %a4 to i16
  %b16 = sext i4 %b4 to i16
  %product = mul i16 %a16, %b16
  ret i16 %product
}

; One byte operand does not justify narrowing a general word multiply.
define i16 @byte_times_word(i8 signext %a, i16 %b) {
; CHECK-LABEL: byte_times_word:
; CHECK: mul16
; CHECK: ret
  %a16 = sext i8 %a to i16
  %product = mul i16 %a16, %b
  ret i16 %product
}

define i16 @general_words(i16 %a, i16 %b) {
; CHECK-LABEL: general_words:
; CHECK: mul16
; CHECK: ret
  %product = mul i16 %a, %b
  ret i16 %product
}

; Register allocation must retain the byte opcode when hoisted word carriers
; spill. Each sample is distinct so the products cannot be factored together.
define i16 @spilled_signed_coefficients(ptr %samples, ptr %coefficients, i16 %count) {
; CHECK-LABEL: spilled_signed_coefficients:
; CHECK: .LBB
; CHECK: ldsp16
; CHECK: muls8.w
; CHECK-NOT: mul16
; CHECK: ret
; ISEL-LABEL: name: spilled_signed_coefficients
; ISEL: bb.1.loop:
; ISEL-NOT: SEXT8_PSEUDO
; ISEL-COUNT-6: MULS8W_PSEUDO
; ISEL-NOT: SEXT8_PSEUDO
; ISEL: RET_PSEUDO
entry:
  %cp0 = getelementptr i8, ptr %coefficients, i16 0
  %c0 = load volatile i8, ptr %cp0
  %cw0 = sext i8 %c0 to i16
  %cp1 = getelementptr i8, ptr %coefficients, i16 1
  %c1 = load volatile i8, ptr %cp1
  %cw1 = sext i8 %c1 to i16
  %cp2 = getelementptr i8, ptr %coefficients, i16 2
  %c2 = load volatile i8, ptr %cp2
  %cw2 = sext i8 %c2 to i16
  %cp3 = getelementptr i8, ptr %coefficients, i16 3
  %c3 = load volatile i8, ptr %cp3
  %cw3 = sext i8 %c3 to i16
  %cp4 = getelementptr i8, ptr %coefficients, i16 4
  %c4 = load volatile i8, ptr %cp4
  %cw4 = sext i8 %c4 to i16
  %cp5 = getelementptr i8, ptr %coefficients, i16 5
  %c5 = load volatile i8, ptr %cp5
  %cw5 = sext i8 %c5 to i16
  br label %loop
loop:
  %index = phi i16 [0, %entry], [%next, %loop]
  %sum = phi i16 [0, %entry], [%total5, %loop]
  %base = getelementptr i8, ptr %samples, i16 %index
  %sp0 = getelementptr i8, ptr %base, i16 0
  %s0 = load volatile i8, ptr %sp0
  %sw0 = sext i8 %s0 to i16
  %p0 = mul i16 %cw0, %sw0
  %total0 = add i16 %sum, %p0
  %sp1 = getelementptr i8, ptr %base, i16 1
  %s1 = load volatile i8, ptr %sp1
  %sw1 = sext i8 %s1 to i16
  %p1 = mul i16 %cw1, %sw1
  %total1 = add i16 %total0, %p1
  %sp2 = getelementptr i8, ptr %base, i16 2
  %s2 = load volatile i8, ptr %sp2
  %sw2 = sext i8 %s2 to i16
  %p2 = mul i16 %cw2, %sw2
  %total2 = add i16 %total1, %p2
  %sp3 = getelementptr i8, ptr %base, i16 3
  %s3 = load volatile i8, ptr %sp3
  %sw3 = sext i8 %s3 to i16
  %p3 = mul i16 %cw3, %sw3
  %total3 = add i16 %total2, %p3
  %sp4 = getelementptr i8, ptr %base, i16 4
  %s4 = load volatile i8, ptr %sp4
  %sw4 = sext i8 %s4 to i16
  %p4 = mul i16 %cw4, %sw4
  %total4 = add i16 %total3, %p4
  %sp5 = getelementptr i8, ptr %base, i16 5
  %s5 = load volatile i8, ptr %sp5
  %sw5 = sext i8 %s5 to i16
  %p5 = mul i16 %cw5, %sw5
  %total5 = add i16 %total4, %p5
  %next = add i16 %index, 1
  %done = icmp eq i16 %next, %count
  br i1 %done, label %exit, label %loop
exit:
  ret i16 %total5
}

; Replacing a folded sign-extending load must preserve its updated pointer
; result as well as its chain. This also covers the 24-bit program pointer.
define i16 @ram_postincrement(ptr %start, i8 signext %coefficient, i16 %count, ptr %out) {
; CHECK-LABEL: ram_postincrement:
; CHECK-NOT: sext8
; CHECK: muls8.w
; CHECK-NOT: sext8
; CHECK: ret
; ISEL-LABEL: name: ram_postincrement
; ISEL-NOT: SEXT8_PSEUDO
; ISEL: LOAD8U_POST_PSEUDO
; ISEL-NOT: SEXT8_PSEUDO
; ISEL: MULS8W_PSEUDO
; ISEL-NOT: SEXT8_PSEUDO
; ISEL: RET_PSEUDO
entry:
  %cw = sext i8 %coefficient to i16
  br label %loop
loop:
  %cursor = phi ptr [%start, %entry], [%advance, %loop]
  %left = phi i16 [%count, %entry], [%remaining, %loop]
  %sum = phi i16 [0, %entry], [%total, %loop]
  %sample = load volatile i8, ptr %cursor
  %sw = sext i8 %sample to i16
  %product = mul i16 %cw, %sw
  %total = add i16 %sum, %product
  %advance = getelementptr i8, ptr %cursor, i16 1
  %remaining = sub i16 %left, 1
  %done = icmp eq i16 %remaining, 0
  br i1 %done, label %exit, label %loop
exit:
  store ptr %advance, ptr %out
  ret i16 %total
}

; Replacing a folded sign-extending load must preserve its updated pointer
; result as well as its chain. This also covers the 24-bit program pointer.
define i16 @program_postincrement(ptr addrspace(1) %start, i8 signext %coefficient, i16 %count, ptr %out) {
; CHECK-LABEL: program_postincrement:
; CHECK-NOT: sext8
; CHECK: muls8.w
; CHECK-NOT: sext8
; CHECK: ret
; ISEL-LABEL: name: program_postincrement
; ISEL-NOT: SEXT8_PSEUDO
; ISEL: PLOAD8U_POST_PSEUDO
; ISEL-NOT: SEXT8_PSEUDO
; ISEL: MULS8W_PSEUDO
; ISEL-NOT: SEXT8_PSEUDO
; ISEL: RET_PSEUDO
entry:
  %cw = sext i8 %coefficient to i16
  br label %loop
loop:
  %cursor = phi ptr addrspace(1) [%start, %entry], [%advance, %loop]
  %left = phi i16 [%count, %entry], [%remaining, %loop]
  %sum = phi i16 [0, %entry], [%total, %loop]
  %sample = load volatile i8, ptr addrspace(1) %cursor
  %sw = sext i8 %sample to i16
  %product = mul i16 %cw, %sw
  %total = add i16 %sum, %product
  %advance = getelementptr i8, ptr addrspace(1) %cursor, i16 1
  %remaining = sub i16 %left, 1
  %done = icmp eq i16 %remaining, 0
  br i1 %done, label %exit, label %loop
exit:
  store ptr addrspace(1) %advance, ptr %out
  ret i16 %total
}
