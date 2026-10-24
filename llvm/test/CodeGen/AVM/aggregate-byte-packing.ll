; RUN: llc -mtriple=avm -O2 -verify-machineinstrs %s -o - | FileCheck %s
; RUN: llc -mtriple=avm -O2 -stop-after=finalize-isel -verify-machineinstrs %s -o - | FileCheck %s --check-prefix=ISEL --implicit-check-not=OR32_PSEUDO --implicit-check-not=BUILD_HI16_PSEUDO
; RUN: llc -mtriple=avm -O2 -verify-machineinstrs -filetype=obj %s -o %t.o

; Small aggregate ABI values are integers with two bytes per physical word.
; Check native word construction before allocation and all late passes, too.

define i16 @pack2(i8 zeroext %a, i8 zeroext %b) {
; CHECK-LABEL: pack2:
; CHECK: lsl16i r5, 8
; CHECK-NEXT: or r4, r5
; CHECK-NEXT: ret
; ISEL-LABEL: name: pack2
; ISEL: LSL16I_PSEUDO
; ISEL-NEXT: {{.*}}OR16_PSEUDO
; ISEL-NEXT: $r4 = COPY
  %lo = zext i8 %a to i16
  %hi = zext i8 %b to i16
  %shift = shl i16 %hi, 8
  %result = or i16 %lo, %shift
  ret i16 %result
}

; This is Clang's i24 coercion of ChunkCoord {area, x, y}.
define i24 @pack3(i8 zeroext %a, i8 zeroext %b, i8 zeroext %c) {
; CHECK-LABEL: pack3:
; CHECK-NOT: xor
; CHECK: lsl16i {{r[4-7]}}, 8
; CHECK: or
; CHECK-NOT: or
; CHECK-NOT: lsl16i
; CHECK-NOT: xor
; CHECK: ret
; ISEL-LABEL: name: pack3
; ISEL: %[[C:[0-9]+]]:gpr16 = COPY $r6
; ISEL-NOT: ZEXT16_32_PSEUDO
; ISEL: %[[SHIFT:[0-9]+]]:uppergpr16 = LSL16I_PSEUDO {{.*}}, 8
; ISEL-NEXT: %[[LOW:[0-9]+]]:gpr16 = OR16_PSEUDO %[[SHIFT]],
; ISEL-NEXT: {{.*}}BUILD_PAIR_PSEUDO killed %[[LOW]], %[[C]]
; ISEL-NEXT: $r4r5 = COPY
  %lo = zext i8 %a to i24
  %mid = zext i8 %b to i24
  %hi = zext i8 %c to i24
  %midshift = shl i24 %mid, 8
  %hishift = shl i24 %hi, 16
  %pair = or i24 %lo, %midshift
  %result = or i24 %pair, %hishift
  ret i24 %result
}

define i32 @pack4(i8 zeroext %a, i8 zeroext %b, i8 zeroext %c, i8 zeroext %d) {
; CHECK-LABEL: pack4:
; CHECK-NOT: xor
; CHECK: lsl16i {{r[4-7]}}, 8
; CHECK: or
; CHECK: lsl16i {{r[4-7]}}, 8
; CHECK: or
; CHECK-NOT: or
; CHECK-NOT: lsl16i
; CHECK-NOT: xor
; CHECK: ret
; ISEL-LABEL: name: pack4
; ISEL-NOT: ZEXT16_32_PSEUDO
; ISEL: LSL16I_PSEUDO
; ISEL-NEXT: {{.*}}OR16_PSEUDO
; ISEL-NEXT: {{.*}}LSL16I_PSEUDO
; ISEL-NEXT: {{.*}}OR16_PSEUDO
; ISEL-NEXT: {{.*}}BUILD_PAIR_PSEUDO
; ISEL-NEXT: $r4r5 = COPY
  %aa = zext i8 %a to i32
  %bb = zext i8 %b to i32
  %cc = zext i8 %c to i32
  %dd = zext i8 %d to i32
  %bs = shl i32 %bb, 8
  %cs = shl i32 %cc, 16
  %ds = shl i32 %dd, 24
  %ab = or i32 %aa, %bs
  %cd = or i32 %cs, %ds
  %result = or i32 %ab, %cd
  ret i32 %result
}

; Wider inputs must still be truncated before their fields are combined.
define i24 @pack3_words(i16 %a, i16 %b, i16 %c) {
; CHECK-LABEL: pack3_words:
; CHECK-NOT: xor
; CHECK: lsl16i {{r[4-7]}}, 8
; CHECK: zext8
; CHECK: or
; CHECK-NOT: or
; CHECK-NOT: lsl16i
; CHECK: ret
; ISEL-LABEL: name: pack3_words
; ISEL: LSL16I_PSEUDO
; ISEL-NEXT: {{.*}}ZEXT8_PSEUDO
; ISEL-NEXT: {{.*}}OR16_PSEUDO
; ISEL-NEXT: {{.*}}BUILD_PAIR_PSEUDO
  %a8 = trunc i16 %a to i8
  %b8 = trunc i16 %b to i8
  %c8 = trunc i16 %c to i8
  %aa = zext i8 %a8 to i24
  %bb = zext i8 %b8 to i24
  %cc = zext i8 %c8 to i24
  %bs = shl i24 %bb, 8
  %cs = shl i24 %cc, 16
  %ab = or i24 %aa, %bs
  %result = or i24 %ab, %cs
  ret i24 %result
}

declare void @consume3(i24)

; Load the area from RAM and coordinates from word stack arguments, like RPG.
define void @call3(ptr %area, i32 %unused, i16 %x, i16 %y) {
; CHECK-LABEL: call3:
; CHECK-NOT: xor
; CHECK-NOT: stsp
; CHECK: ldsp16
; CHECK: ldsp16
; CHECK: ld8u
; CHECK: lsl16i {{r[4-7]}}, 8
; CHECK-NEXT: or
; CHECK-NEXT: mov
; CHECK: jmp consume3
; ISEL-LABEL: name: call3
; ISEL: LSL16I_PSEUDO
; ISEL-NEXT: {{.*}}OR16_PSEUDO
; ISEL-NOT: OR16_PSEUDO
; ISEL: CALL_DIRECT_PSEUDO @consume3
  %a8 = load volatile i8, ptr %area
  %x8 = trunc i16 %x to i8
  %y8 = trunc i16 %y to i8
  %aa = zext i8 %a8 to i24
  %xx = zext i8 %x8 to i24
  %yy = zext i8 %y8 to i24
  %xs = shl i24 %xx, 8
  %ys = shl i24 %yy, 16
  %ax = or i24 %aa, %xs
  %coord = or i24 %ax, %ys
  call void @consume3(i24 %coord)
  ret void
}

; Packing is independent of the OR tree's grouping or byte order.
define i32 @pack4_reversed(i16 %a, i16 %b, i16 %c, i16 %d) {
; CHECK-LABEL: pack4_reversed:
; CHECK-NOT: xor
; CHECK: lsl16i {{r[4-7]}}, 8
; CHECK: zext8
; CHECK: or
; CHECK: lsl16i {{r[4-7]}}, 8
; CHECK: zext8
; CHECK: or
; CHECK-NOT: or
; CHECK-NOT: lsl16i
; CHECK: ret
; ISEL-LABEL: name: pack4_reversed
; ISEL: LSL16I_PSEUDO
; ISEL-NEXT: {{.*}}ZEXT8_PSEUDO
; ISEL-NEXT: {{.*}}OR16_PSEUDO
; ISEL-NEXT: {{.*}}LSL16I_PSEUDO
; ISEL-NEXT: {{.*}}ZEXT8_PSEUDO
; ISEL-NEXT: {{.*}}OR16_PSEUDO
; ISEL-NEXT: {{.*}}BUILD_PAIR_PSEUDO
  %aa = and i16 %a, 255
  %bb = and i16 %b, 255
  %cc = and i16 %c, 255
  %dd = and i16 %d, 255
  %aw = zext i16 %aa to i32
  %bw = zext i16 %bb to i32
  %cw = zext i16 %cc to i32
  %dw = zext i16 %dd to i32
  %as = shl i32 %aw, 24
  %bs = shl i32 %bw, 16
  %cs = shl i32 %cw, 8
  %mixed = or i32 %dw, %bs
  %more = or i32 %mixed, %cs
  %result = or i32 %more, %as
  ret i32 %result
}

; The same lowering applies to any disjoint native words, not just bytes.
define i32 @pack_words(i16 %lo, i16 %hi) {
; CHECK-LABEL: pack_words:
; CHECK-NOT: {{^or[ \t]}}
; CHECK-NOT: {{^xor[ \t]}}
; CHECK-NOT: {{^lsl16i[ \t]}}
; CHECK: ret
; ISEL-LABEL: name: pack_words
; ISEL-NOT: OR16_PSEUDO
; ISEL-NOT: ZEXT16_32_PSEUDO
; ISEL: BUILD_PAIR_PSEUDO
; ISEL-NEXT: $r4r5 = COPY
  %low = zext i16 %lo to i32
  %high = zext i16 %hi to i32
  %shift = shl i32 %high, 16
  %result = or i32 %low, %shift
  ret i32 %result
}
