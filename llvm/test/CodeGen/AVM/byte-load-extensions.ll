; RUN: llc -mtriple=avm -O2 -stop-after=finalize-isel -verify-machineinstrs %s -o - | FileCheck %s
; Check before register allocation and the late cleanup: byte loads themselves
; provide the zero extension required by an outgoing byte argument.

declare i16 @consume(i8 zeroext, i16, i16, i16)
declare i16 @consume_signed(i8 signext)

; SelectionDAG cannot infer the promoted upper bits of an unvisited loop
; backedge. Promote the comparison tree while the IR PHI still records its
; original byte type, rather than masking its register on every iteration.
define i16 @program_byte_loop_phi(ptr addrspace(1) %address, i16 %count) {
; CHECK-LABEL: name: program_byte_loop_phi
; CHECK-NOT: ZEXT8_PSEUDO
; CHECK: PLOAD8U
; CHECK-NOT: ZEXT8_PSEUDO
; CHECK: RET_PSEUDO
entry:
  br label %loop
loop:
  %byte = phi i8 [87, %entry], [%loaded, %next]
  %cursor = phi ptr addrspace(1) [%address, %entry], [%advanced, %next]
  %left = phi i16 [%count, %entry], [%remaining, %next]
  %is_small = icmp ult i8 %byte, 13
  br i1 %is_small, label %exit, label %check
check:
  %done = icmp eq i16 %left, 0
  br i1 %done, label %exit, label %next
next:
  %loaded = load volatile i8, ptr addrspace(1) %cursor
  %advanced = getelementptr i8, ptr addrspace(1) %cursor, i16 1
  %remaining = sub i16 %left, 1
  br label %loop
exit:
  %result = zext i8 %byte to i16
  ret i16 %result
}

define i16 @ram_byte_across_blocks(ptr %address, i1 %condition, i16 %a) {
; CHECK-LABEL: name: ram_byte_across_blocks
; CHECK: LOAD8U_PSEUDO
; CHECK: bb.1
; CHECK-NOT: ZEXT8_PSEUDO
; CHECK: CALL_DIRECT_PSEUDO @consume
  %byte = load volatile i8, ptr %address
  br i1 %condition, label %use, label %skip
use:
  %result = call i16 @consume(i8 zeroext %byte, i16 %a, i16 123, i16 456)
  ret i16 %result
skip:
  ret i16 0
}

define i16 @program_byte_across_blocks(ptr addrspace(1) %address, i1 %condition, i16 %a) {
; CHECK-LABEL: name: program_byte_across_blocks
; CHECK: PLOAD8U_PSEUDO
; CHECK: bb.1
; CHECK-NOT: ZEXT8_PSEUDO
; CHECK: CALL_DIRECT_PSEUDO @consume
  %byte = load volatile i8, ptr addrspace(1) %address
  br i1 %condition, label %use, label %skip
use:
  %result = call i16 @consume(i8 zeroext %byte, i16 %a, i16 123, i16 456)
  ret i16 %result
skip:
  ret i16 0
}

define i16 @ram_byte(ptr %address, i16 %a, i16 %b, i16 %c) {
; CHECK-LABEL: name: ram_byte
; CHECK: LOAD8U_PSEUDO
; CHECK-NOT: ZEXT8_PSEUDO
; CHECK: CALL_DIRECT_PSEUDO @consume
  %byte = load i8, ptr %address
  %result = call i16 @consume(i8 zeroext %byte, i16 %a, i16 %b, i16 %c)
  ret i16 %result
}

define i16 @program_byte(ptr addrspace(1) %address, i16 %a, i16 %b, i16 %c) {
; CHECK-LABEL: name: program_byte
; CHECK: PLOAD8U_PSEUDO
; CHECK-NOT: ZEXT8_PSEUDO
; CHECK: CALL_DIRECT_PSEUDO @consume
  %byte = load i8, ptr addrspace(1) %address
  %result = call i16 @consume(i8 zeroext %byte, i16 %a, i16 %b, i16 %c)
  ret i16 %result
}

define i16 @ram_byte_two_extensions(ptr %address) {
; CHECK-LABEL: name: ram_byte_two_extensions
; CHECK: LOAD8U_PSEUDO
; CHECK-NOT: ZEXT8_PSEUDO
; CHECK: SEXT8_PSEUDO
; CHECK: ZEXT8_PSEUDO
; CHECK: ADD16_PSEUDO
  %byte = load i8, ptr %address
  %unsigned = zext i8 %byte to i16
  %signed = sext i8 %byte to i16
  %result = add i16 %unsigned, %signed
  ret i16 %result
}

define i16 @signed_argument(ptr %address) {
; CHECK-LABEL: name: signed_argument
; CHECK: LOAD8U_PSEUDO
; CHECK: SEXT8_PSEUDO
; CHECK: CALL_DIRECT_PSEUDO @consume_signed
  %byte = load i8, ptr %address
  %result = call i16 @consume_signed(i8 signext %byte)
  ret i16 %result
}

define i16 @truncated_word(i16 %word) {
; CHECK-LABEL: name: truncated_word
; CHECK: ZEXT8_PSEUDO
; CHECK: CALL_DIRECT_PSEUDO @consume
  %byte = trunc i16 %word to i8
  %result = call i16 @consume(i8 zeroext %byte, i16 0, i16 0, i16 0)
  ret i16 %result
}

define void @ram_indexed_loop(ptr %address, ptr %updated, i16 %count) {
; CHECK-LABEL: name: ram_indexed_loop
; CHECK: LOAD8U_POST_PSEUDO
; CHECK-NOT: ZEXT8_PSEUDO
; CHECK: CALL_DIRECT_PSEUDO @consume
entry:
  br label %loop
loop:
  %cursor = phi ptr [%address, %entry], [%next, %use]
  %left = phi i16 [%count, %entry], [%remaining, %use]
  %byte = load volatile i8, ptr %cursor
  %next = getelementptr i8, ptr %cursor, i16 1
  store volatile ptr %next, ptr %updated
  %done = icmp eq i16 %left, 0
  br i1 %done, label %exit, label %use
use:
  %result = call i16 @consume(i8 zeroext %byte, i16 %left, i16 123, i16 456)
  %remaining = sub i16 %left, 1
  br label %loop
exit:
  ret void
}

define void @program_indexed_loop(ptr addrspace(1) %address, ptr %updated, i16 %count) {
; CHECK-LABEL: name: program_indexed_loop
; CHECK: PLOAD8U_POST_PSEUDO
; CHECK-NOT: ZEXT8_PSEUDO
; CHECK: CALL_DIRECT_PSEUDO @consume
entry:
  br label %loop
loop:
  %cursor = phi ptr addrspace(1) [%address, %entry], [%next, %use]
  %left = phi i16 [%count, %entry], [%remaining, %use]
  %byte = load volatile i8, ptr addrspace(1) %cursor
  %next = getelementptr i8, ptr addrspace(1) %cursor, i16 1
  store volatile ptr addrspace(1) %next, ptr %updated
  %done = icmp eq i16 %left, 0
  br i1 %done, label %exit, label %use
use:
  %result = call i16 @consume(i8 zeroext %byte, i16 %left, i16 123, i16 456)
  %remaining = sub i16 %left, 1
  br label %loop
exit:
  ret void
}
