; RUN: llc -mtriple=avm -O2 -verify-machineinstrs %s -o - | FileCheck %s
; RUN: llc -mtriple=avm -O2 -stop-after=twoaddressinstruction -verify-machineinstrs %s -o - | FileCheck %s --check-prefix=TWOADDR

@bytes = external addrspace(1) constant [8 x i8], align 1

; Preserve the invariant 24-bit base and add it to the dead, widened index.
; Tying the result to the base instead requires a third pair and forces the
; allocator to rematerialize the symbol on every iteration.
define i16 @scan_program() {
; CHECK-LABEL: scan_program:
; CHECK: ldi16 {{r[0-7]}}, %lo16(bytes)
; CHECK-NEXT: ldi8 {{r[0-7]}}, %hi8(bytes)
; CHECK: .LBB0_1:
; CHECK-NOT: %lo16(bytes)
; CHECK-NOT: %hi8(bytes)
; CHECK-NOT: ldsp
; CHECK-NOT: stsp
; CHECK: add32
; CHECK-NEXT: ldp8u
; CHECK-NOT: %lo16(bytes)
; CHECK-NOT: %hi8(bytes)
; CHECK: ret
; TWOADDR-LABEL: name: scan_program
; TWOADDR: %[[BASE:[0-9]+]]:gpr32 = PROG_ADDR_PSEUDO @bytes
; TWOADDR: %[[OFFSET:[0-9]+]]:gpr32 = ZEXT16_32_PSEUDO
; TWOADDR-NEXT: %[[ADDRESS:[0-9]+]]:gpr32 = COPY killed %[[OFFSET]]
; TWOADDR-NEXT: %[[ADDRESS]]:gpr32 = nuw PROG_ADD_PSEUDO %[[ADDRESS]], %[[BASE]]
entry:
  br label %loop
loop:
  %i = phi i16 [0, %entry], [%next, %loop]
  %sum = phi i16 [0, %entry], [%sum.next, %loop]
  %byte = phi i8 [-1, %entry], [%loaded, %loop]
  %small = icmp ult i8 %byte, 32
  %increment = zext i1 %small to i16
  %sum.next = add i16 %sum, %increment
  %index = and i16 %i, 7
  %wide = zext i16 %index to i24
  %address = getelementptr inbounds i8, ptr addrspace(1) @bytes, i24 %wide
  %loaded = load volatile i8, ptr addrspace(1) %address, align 1
  %next = add nuw nsw i16 %i, 1
  %done = icmp eq i16 %next, 65
  br i1 %done, label %exit, label %loop
exit:
  ret i16 %sum.next
}

; Both full-width operands remain live after addition. Commutation must not
; discard high address/offset bits or overwrite either source's other uses.
define i32 @live_wide_offset(ptr addrspace(1) %base, i32 %offset) {
; CHECK-LABEL: live_wide_offset:
; CHECK-COUNT-2: add32
; CHECK: ldp8u
; CHECK: ret
  %address = getelementptr i8, ptr addrspace(1) %base, i32 %offset
  %byte = load volatile i8, ptr addrspace(1) %address, align 1
  %bits = ptrtoint ptr addrspace(1) %base to i32
  %sum = add i32 %bits, %offset
  %extended = zext i8 %byte to i32
  %result = xor i32 %sum, %extended
  ret i32 %result
}

; Keeping two wide accumulators, two words, and a scalar index live leaves
; insufficient pairs for the invariant base. Retain rematerialization as the
; register allocator's pressure fallback rather than forcing the base live.
define i32 @crowded_scan() {
; CHECK-LABEL: crowded_scan:
; CHECK: =>This Inner Loop Header:
; CHECK: ldi16 {{r[0-7]}}, %lo16(bytes)
; CHECK-NEXT: ldi8 {{r[0-7]}}, %hi8(bytes)
; CHECK: add32
; CHECK: ldp8u
; CHECK: ret
entry:
  br label %loop
loop:
  %i = phi i16 [0, %entry], [%next, %loop]
  %a = phi i32 [305419896, %entry], [%a.next, %loop]
  %b = phi i32 [-2023406815, %entry], [%b.next, %loop]
  %c = phi i16 [4951, %entry], [%c.next, %loop]
  %d = phi i16 [9320, %entry], [%d.next, %loop]
  %index = and i16 %i, 7
  %wide = zext i16 %index to i24
  %address = getelementptr inbounds i8, ptr addrspace(1) @bytes, i24 %wide
  %value = load volatile i8, ptr addrspace(1) %address, align 1
  %value.wide = zext i8 %value to i32
  %value.word = zext i8 %value to i16
  %a.next = add i32 %a, %value.wide
  %b.next = xor i32 %b, %a.next
  %c.next = add i16 %c, %value.word
  %d.next = xor i16 %d, %c.next
  %next = add nuw nsw i16 %i, 1
  %done = icmp eq i16 %next, 65
  br i1 %done, label %exit, label %loop
exit:
  %ab = xor i32 %a.next, %b.next
  %cd = xor i16 %c.next, %d.next
  %cd.wide = zext i16 %cd to i32
  %result = xor i32 %ab, %cd.wide
  ret i32 %result
}
