; RUN: llc -mtriple=avm-unknown-arduboyfx -mcpu=avm1 \
; RUN:   -mtune=avm-interpreter-32u4-v1 -O0 -verify-machineinstrs < %s \
; RUN:   | FileCheck %s

define void @frame127(i8 %x) {
; CHECK-LABEL: frame127:
; CHECK:       adjsp -127
; CHECK:       stsp8 [sp+126], r4
; CHECK-NEXT:  adjsp 127
; CHECK-NEXT:  ret
  %a = alloca [127 x i8], align 1
  %p = getelementptr [127 x i8], ptr %a, i16 0, i16 126
  store volatile i8 %x, ptr %p, align 1
  ret void
}

define void @frame128(i8 %x) {
; CHECK-LABEL: frame128:
; CHECK:       adjsp -128
; CHECK:       stsp8 [sp+127], r4
; CHECK-NEXT:  adjsp 127
; CHECK-NEXT:  adjsp 1
; CHECK-NEXT:  ret
  %a = alloca [128 x i8], align 1
  %p = getelementptr [128 x i8], ptr %a, i16 0, i16 127
  store volatile i8 %x, ptr %p, align 1
  ret void
}

define i16 @frame_pointer(i16 %x) #0 {
; CHECK-LABEL: frame_pointer:
; CHECK:       push16 r3
; CHECK-NEXT:  adjsp -4
; CHECK-NEXT:  getsp r3
; CHECK:       setsp r3
; CHECK-NEXT:  adjsp 4
; CHECK-NEXT:  pop16 r3
; CHECK-NEXT:  ret
  %a = alloca i16, align 1
  store volatile i16 %x, ptr %a, align 1
  %v = load volatile i16, ptr %a, align 1
  ret i16 %v
}

declare void @take(ptr)

define void @frame_address() {
; CHECK-LABEL: frame_address:
; CHECK:       adjsp -8
; CHECK-NEXT:  leasp r4, 5
; CHECK-NEXT:  call take
; CHECK-NEXT:  adjsp 8
; CHECK-NEXT:  ret
  %a = alloca [8 x i8], align 1
  %p = getelementptr [8 x i8], ptr %a, i16 0, i16 5
  call void @take(ptr %p)
  ret void
}

define i16 @fp_incoming(i16 %a, i16 %b, i16 %c, i16 %d, i16 %e,
                        i16 %f) #0 {
; CHECK-LABEL: fp_incoming:
; CHECK:       push16 r3
; CHECK-NEXT:  adjsp -2
; CHECK-NEXT:  getsp r3
; CHECK:       addi.s8 r4, 9
; CHECK-NEXT:  ld16 r4, [r4]
; CHECK:       setsp r3
; CHECK-NEXT:  adjsp 2
; CHECK-NEXT:  pop16 r3
  ret i16 %f
}

attributes #0 = { "frame-pointer"="all" }
