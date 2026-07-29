# RUN: llvm-mc -triple=avm -show-encoding %s | FileCheck %s
# RUN: llvm-mc -triple=avm -filetype=obj %s -o %t
# RUN: llvm-objdump -d %t | FileCheck %s --check-prefix=DIS

sys display
# CHECK: sys display{{.*}}encoding: [0xd7,0x1d]
# DIS: sys display

sys draw_sprite_overwrite
# CHECK: sys draw_sprite_overwrite{{.*}}encoding: [0xd7,0x1e]
# DIS: sys draw_sprite_overwrite

sys draw_sprite_plus_mask
# CHECK: sys draw_sprite_plus_mask{{.*}}encoding: [0xd7,0x1f]
# DIS: sys draw_sprite_plus_mask

sys draw_sprite_self_masked
# CHECK: sys draw_sprite_self_masked{{.*}}encoding: [0xd7,0x20]
# DIS: sys draw_sprite_self_masked

sys draw_sprite_erase
# CHECK: sys draw_sprite_erase{{.*}}encoding: [0xd7,0x21]
# DIS: sys draw_sprite_erase

sys draw_filled_rect_white
# CHECK: sys draw_filled_rect_white{{.*}}encoding: [0xd7,0x27]
# DIS: sys draw_filled_rect_white

sys draw_filled_rect_black
# CHECK: sys draw_filled_rect_black{{.*}}encoding: [0xd7,0x28]
# DIS: sys draw_filled_rect_black

sys vsnprintf
# CHECK: sys vsnprintf{{.*}}encoding: [0xd7,0x2f]
# DIS: sys vsnprintf

sys vsnprintf_p
# CHECK: sys vsnprintf_p{{.*}}encoding: [0xd7,0x30]
# DIS: sys vsnprintf_p
