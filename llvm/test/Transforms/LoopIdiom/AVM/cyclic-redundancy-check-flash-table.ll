; RUN: opt -mtriple=avm-unknown-arduboyfx -mcpu=avm1 \
; RUN:   -mtune=avm-interpreter-32u4-v1 -passes=loop-idiom -S %s | FileCheck %s

; On AVM a table entry is one flash-window load, which costs far less than the bit-serial
; loop it replaces, so each recognized CRC becomes a table lookup. The table is an immutable
; AS0 constant that the object file places in the flash window. The four shapes cover
; little-endian and big-endian CRCs, 8-bit and 16-bit and 32-bit CRC widths, and a data byte
; XOR'ed into the index.

; CHECK-DAG: @.crctable{{.*}} = private constant [256 x i16]
; CHECK-DAG: @.crctable{{.*}} = private constant [256 x i32]
; CHECK-DAG: @.crctable{{.*}} = private constant [256 x i8]

define i16 @crc16.le.tc8(i8 %msg, i16 %checksum) {
; CHECK-LABEL: define i16 @crc16.le.tc8(
; CHECK:       %tbl.ld = load i16, ptr %tbl.ptradd, align 1
entry:
  br label %loop

loop:
  %iv = phi i8 [ 0, %entry ], [ %iv.next, %loop ]
  %crc = phi i16 [ %checksum, %entry ], [ %crc.next, %loop ]
  %data = phi i8 [ %msg, %entry ], [ %data.next, %loop ]
  %crc.trunc = trunc i16 %crc to i8
  %xor.data.crc = xor i8 %data, %crc.trunc
  %and.data.crc = and i8 %xor.data.crc, 1
  %data.next = lshr i8 %data, 1
  %check.sb = icmp eq i8 %and.data.crc, 0
  %crc.lshr = lshr i16 %crc, 1
  %xor = xor i16 %crc.lshr, -24575
  %crc.next = select i1 %check.sb, i16 %crc.lshr, i16 %xor
  %iv.next = add nuw nsw i8 %iv, 1
  %exit.cond = icmp samesign ult i8 %iv, 7
  br i1 %exit.cond, label %loop, label %exit

exit:
  ret i16 %crc.next
}

define i32 @crc32.le.tc8(i8 %msg, i32 %checksum) {
; CHECK-LABEL: define i32 @crc32.le.tc8(
; CHECK:       %tbl.ld = load i32, ptr %tbl.ptradd, align 1
entry:
  br label %loop

loop:
  %iv = phi i8 [ 0, %entry ], [ %iv.next, %loop ]
  %crc = phi i32 [ %checksum, %entry ], [ %crc.next, %loop ]
  %data = phi i8 [ %msg, %entry ], [ %data.next, %loop ]
  %crc.trunc = trunc i32 %crc to i8
  %xor.data.crc = xor i8 %data, %crc.trunc
  %and.data.crc = and i8 %xor.data.crc, 1
  %data.next = lshr i8 %data, 1
  %check.sb = icmp eq i8 %and.data.crc, 0
  %crc.lshr = lshr i32 %crc, 1
  %xor = xor i32 %crc.lshr, -306674912
  %crc.next = select i1 %check.sb, i32 %crc.lshr, i32 %xor
  %iv.next = add nuw nsw i8 %iv, 1
  %exit.cond = icmp samesign ult i8 %iv, 7
  br i1 %exit.cond, label %loop, label %exit

exit:
  ret i32 %crc.next
}

define i8 @crc8.le.tc16(i16 %msg, i8 %checksum) {
; CHECK-LABEL: define i8 @crc8.le.tc16(
; CHECK:       %tbl.ld = load i8, ptr %tbl.ptradd, align 1
entry:
  br label %loop

loop:
  %iv = phi i8 [ 0, %entry ], [ %iv.next, %loop ]
  %crc = phi i8 [ %checksum, %entry ], [ %crc.next, %loop ]
  %data = phi i16 [ %msg, %entry ], [ %data.next, %loop ]
  %data.trunc = trunc i16 %data to i8
  %xor.crc.data = xor i8 %crc, %data.trunc
  %and.crc.data = and i8 %xor.crc.data, 1
  %data.next = lshr i16 %data, 1
  %check.sb = icmp eq i8 %and.crc.data, 0
  %crc.lshr = lshr i8 %crc, 1
  %crc.xor = xor i8 %crc.lshr, 29
  %crc.next = select i1 %check.sb, i8 %crc.lshr, i8 %crc.xor
  %iv.next = add nuw nsw i8 %iv, 1
  %exit.cond = icmp samesign ult i8 %iv, 15
  br i1 %exit.cond, label %loop, label %exit

exit:
  ret i8 %crc.next
}

define i16 @crc16.be.tc8.crc.init.li(i16 %checksum, i8 %msg) {
; CHECK-LABEL: define i16 @crc16.be.tc8.crc.init.li(
; CHECK:       %tbl.ld = load i16, ptr %tbl.ptradd, align 1
entry:
  %msg.ext = zext i8 %msg to i16
  %msg.shl = shl nuw i16 %msg.ext, 8
  %crc.init = xor i16 %msg.shl, %checksum
  br label %loop

loop:
  %iv = phi i32 [ 0, %entry ], [ %iv.next, %loop ]
  %crc = phi i16 [ %crc.init, %entry ], [ %crc.next, %loop ]
  %crc.shl = shl i16 %crc, 1
  %crc.xor = xor i16 %crc.shl, 4129
  %check.sb = icmp slt i16 %crc, 0
  %crc.next = select i1 %check.sb, i16 %crc.xor, i16 %crc.shl
  %iv.next = add nuw nsw i32 %iv, 1
  %exit.cond = icmp samesign ult i32 %iv, 7
  br i1 %exit.cond, label %loop, label %exit

exit:
  ret i16 %crc.next
}
