; RUN: llc -mtriple=avm-unknown-arduboyfx -mcpu=avm1 \
; RUN:   -mtune=avm-interpreter-32u4-v1 -O2 -verify-machineinstrs < %s \
; RUN:   | FileCheck %s
; RUN: llc -mtriple=avm-unknown-arduboyfx -mcpu=avm1 \
; RUN:   -mtune=avm-interpreter-32u4-v1 -O2 -verify-machineinstrs \
; RUN:   -filetype=obj < %s -o %t.o

declare void @llvm.avm.debug.putc(i8)

define i16 @debug_putc_register_pressure(ptr %p0, ptr %p1, ptr %p2, ptr %p3,
                                         ptr %p4, ptr %p5, ptr %p6, ptr %p7) #0 {
; CHECK-LABEL: debug_putc_register_pressure:
; CHECK-COUNT-8: sys debug_putc
  %v0 = load volatile i8, ptr %p0
  %v1 = load volatile i8, ptr %p1
  %v2 = load volatile i8, ptr %p2
  %v3 = load volatile i8, ptr %p3
  %v4 = load volatile i8, ptr %p4
  %v5 = load volatile i8, ptr %p5
  %v6 = load volatile i8, ptr %p6
  %v7 = load volatile i8, ptr %p7

  call void @llvm.avm.debug.putc(i8 %v0)
  call void @llvm.avm.debug.putc(i8 %v1)
  call void @llvm.avm.debug.putc(i8 %v2)
  call void @llvm.avm.debug.putc(i8 %v3)
  call void @llvm.avm.debug.putc(i8 %v4)
  call void @llvm.avm.debug.putc(i8 %v5)
  call void @llvm.avm.debug.putc(i8 %v6)
  call void @llvm.avm.debug.putc(i8 %v7)

  %z0 = zext i8 %v0 to i16
  %z1 = zext i8 %v1 to i16
  %z2 = zext i8 %v2 to i16
  %z3 = zext i8 %v3 to i16
  %z4 = zext i8 %v4 to i16
  %z5 = zext i8 %v5 to i16
  %z6 = zext i8 %v6 to i16
  %z7 = zext i8 %v7 to i16
  %a0 = add i16 %z0, %z1
  %a1 = xor i16 %z2, %z3
  %a2 = add i16 %z4, %z5
  %a3 = xor i16 %z6, %z7
  %b0 = xor i16 %a0, %a1
  %b1 = add i16 %a2, %a3
  %result = xor i16 %b0, %b1
  ret i16 %result
}

attributes #0 = { optsize }
