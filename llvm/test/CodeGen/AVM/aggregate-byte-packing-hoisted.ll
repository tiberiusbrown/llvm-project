; RUN: llc -mtriple=avm -O2 -stop-after=finalize-isel -verify-machineinstrs %s -o - | FileCheck %s --implicit-check-not=OR32_PSEUDO
; RUN: llc -mtriple=avm -O2 -verify-machineinstrs -filetype=obj %s -o %t.o

declare void @consume3(i24)

; The shifted y is exported by the outer block, like RPG's nested chunk loops.
; Its low word is still zero when the inner block imports it from a register.
define void @pack3_hoisted(ptr %area, i8 zeroext %y, i16 %count) {
; CHECK-LABEL: name: pack3_hoisted
; CHECK: bb.1.loop:
; CHECK: LSL16I_PSEUDO {{.*}}, 8
; CHECK: OR16_PSEUDO
; CHECK-NOT: OR16_PSEUDO
; CHECK: CALL_DIRECT_PSEUDO @consume3
entry:
  %yw = zext i8 %y to i24
  %ys = shl i24 %yw, 16
  br label %loop
loop:
  %left = phi i16 [%count, %entry], [%remaining, %loop]
  %x8 = trunc i16 %left to i8
  %xw = zext i8 %x8 to i24
  %xs = shl i24 %xw, 8
  %a = load volatile i8, ptr %area
  %aw = zext i8 %a to i24
  %xy = or i24 %xs, %ys
  %coord = or i24 %xy, %aw
  call void @consume3(i24 %coord)
  %remaining = sub i16 %left, 1
  %more = icmp ne i16 %remaining, 0
  br i1 %more, label %loop, label %exit
exit:
  ret void
}
