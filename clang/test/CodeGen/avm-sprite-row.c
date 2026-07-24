// RUN: %clang_cc1 -triple avm-unknown-arduboyfx -target-cpu avm1 \
// RUN:   -tune-cpu avm-interpreter-32u4-v1 -ffreestanding -O2 -S \
// RUN:   -mllvm -verify-machineinstrs -o - %s | FileCheck %s --check-prefix=SHAPE
// RUN: %clang_cc1 -triple avm-unknown-arduboyfx -target-cpu avm1 \
// RUN:   -tune-cpu avm-interpreter-32u4-v1 -ffreestanding -O2 -S \
// RUN:   -mllvm -verify-machineinstrs -o - %s | FileCheck %s --check-prefix=COUNT

#define AS1 __attribute__((address_space(1)))

short offx = 3;
short offy = 2;

static const unsigned char AS1 sprite[] = {
    8, 8, 0x3c, 0x7e, 0xdb, 0xff, 0xff, 0xdb, 0x7e, 0x3c,
};

// SHAPE-LABEL: sprite_rows:
// SHAPE-NOT:   stsp
// SHAPE-NOT:   ldsp
// SHAPE:       ldm16 [[OFFY:r[0-7]]], [offy]
// SHAPE:       ldm16 [[OFFX:r[0-7]]], [offx]
// SHAPE:       ldi16 r6, %lo16(sprite)
// SHAPE-NEXT:  ldi8 r7, %hi8(sprite)
// SHAPE:       [[ROW:LBB[0-9]+_[0-9]+]]:
// SHAPE-NOT:   ldm16
// SHAPE:       mov r5,
// SHAPE-NOT:   mov r5,
// SHAPE:       mov r4, [[OFFX]]
// SHAPE-NEXT:  sys draw_sprite_overwrite
// SHAPE-COUNT-15: addi.s8 r4, 8
// SHAPE-NEXT:  sys draw_sprite_overwrite
// SHAPE-NOT:   mov r4,
// SHAPE-NOT:   sys draw_sprite_overwrite
// SHAPE:       brne [[ROW]]
// SHAPE:       ret
//
// COUNT-LABEL: sprite_rows:
// COUNT-COUNT-2: ldm16
// COUNT-COUNT-16: sys draw_sprite_overwrite
// COUNT: ret
void sprite_rows(void) {
  for (short y = 0; y != 64; y += 8)
    for (short x = 0; x != 128; x += 8)
      __avm_draw_sprite_overwrite(offx + x, offy + y, sprite, 0);
}
