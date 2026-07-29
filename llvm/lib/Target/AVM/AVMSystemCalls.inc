//===-- AVMSystemCalls.inc - AVM system-service metadata ------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// The AVM system-service metadata is intentionally an X-macro table rather
// than an LLVM header.  MC users only need the numeric ID and assembly name,
// whereas CodeGen users also need the neutral stems for the machine pseudo,
// LLVM intrinsic, and cycle-model entry.  Keeping the stems as tokens avoids
// making the MC layer depend on AVM CodeGen enums or LLVM IR declarations.
//
// Consumers define AVM_SYS_DEF as:
//
//   AVM_SYS_DEF(ID, AsmName, PseudoKind, Pseudo, IntrinsicKind, Intrinsic,
//               CostKind, Cost)
//
// `PseudoKind`, `IntrinsicKind`, and `CostKind` are selector macros.  A
// consumer can define AVM_SYS_PSEUDO, AVM_NO_PSEUDO, AVM_SYS_INTRINSIC,
// AVM_NO_INTRINSIC, AVM_SYS_FIXED_COST, AVM_SYS_RANGE_COST, and AVM_NO_COST
// to expand only the relationships it owns.  The AVM_NO_* selectors make
// entries without a semantic pseudo, target intrinsic, or dedicated TTI cost
// representable without introducing a bogus enum dependency.
//
// This is the authoritative LLVM-side numeric ID list.  AVMInstrInfo.td,
// IntrinsicsAVM.td, and Clang builtin declarations remain separate because
// TableGen and Clang need type and register-effect declarations that do not
// fit this MC-safe metadata layer.
//
//===----------------------------------------------------------------------===//

AVM_SYS_DEF(0x00, debug_putc, AVM_SYS_PSEUDO, SYS_DEBUG_PUTC_PSEUDO,
            AVM_SYS_INTRINSIC, avm_debug_putc, AVM_SYS_FIXED_COST,
            SysDebugPutc)
AVM_SYS_DEF(0x01, debug_break, AVM_SYS_PSEUDO, SYS_DEBUG_BREAK_PSEUDO,
            AVM_SYS_INTRINSIC, avm_debug_break, AVM_SYS_FIXED_COST,
            SysDebugBreak)
AVM_SYS_DEF(0x02, millis, AVM_SYS_PSEUDO, SYS_MILLIS_PSEUDO,
            AVM_SYS_INTRINSIC, avm_millis, AVM_SYS_FIXED_COST, SysMillis)
AVM_SYS_DEF(0x03, millis32, AVM_SYS_PSEUDO, SYS_MILLIS32_PSEUDO,
            AVM_SYS_INTRINSIC, avm_millis32, AVM_SYS_FIXED_COST, SysMillis32)
AVM_SYS_DEF(0x04, sinf, AVM_SYS_PSEUDO, SYS_SINF_PSEUDO, AVM_SYS_INTRINSIC,
            avm_sinf, AVM_SYS_RANGE_COST, SysSinf)
AVM_SYS_DEF(0x05, cosf, AVM_SYS_PSEUDO, SYS_COSF_PSEUDO, AVM_SYS_INTRINSIC,
            avm_cosf, AVM_SYS_RANGE_COST, SysCosf)
AVM_SYS_DEF(0x06, atan2f, AVM_SYS_PSEUDO, SYS_ATAN2F_PSEUDO,
            AVM_SYS_INTRINSIC, avm_atan2f, AVM_SYS_RANGE_COST, SysAtan2f)
AVM_SYS_DEF(0x07, tanf, AVM_SYS_PSEUDO, SYS_TANF_PSEUDO, AVM_SYS_INTRINSIC,
            avm_tanf, AVM_SYS_RANGE_COST, SysTanf)
AVM_SYS_DEF(0x08, expf, AVM_SYS_PSEUDO, SYS_EXPF_PSEUDO, AVM_SYS_INTRINSIC,
            avm_expf, AVM_SYS_RANGE_COST, SysExpf)
AVM_SYS_DEF(0x09, logf, AVM_SYS_PSEUDO, SYS_LOGF_PSEUDO, AVM_SYS_INTRINSIC,
            avm_logf, AVM_SYS_RANGE_COST, SysLogf)
AVM_SYS_DEF(0x0a, log2f, AVM_SYS_PSEUDO, SYS_LOG2F_PSEUDO,
            AVM_SYS_INTRINSIC, avm_log2f, AVM_SYS_RANGE_COST, SysLog2f)
AVM_SYS_DEF(0x0b, log10f, AVM_SYS_PSEUDO, SYS_LOG10F_PSEUDO,
            AVM_SYS_INTRINSIC, avm_log10f, AVM_SYS_RANGE_COST, SysLog10f)
AVM_SYS_DEF(0x0c, powf, AVM_SYS_PSEUDO, SYS_POWF_PSEUDO, AVM_SYS_INTRINSIC,
            avm_powf, AVM_SYS_RANGE_COST, SysPowf)
AVM_SYS_DEF(0x0d, hypotf, AVM_SYS_PSEUDO, SYS_HYPOTF_PSEUDO,
            AVM_SYS_INTRINSIC, avm_hypotf, AVM_SYS_RANGE_COST, SysHypotf)
AVM_SYS_DEF(0x0e, fmodf, AVM_SYS_PSEUDO, SYS_FMODF_PSEUDO,
            AVM_SYS_INTRINSIC, avm_fmodf, AVM_SYS_RANGE_COST, SysFmodf)
AVM_SYS_DEF(0x0f, memcpy, AVM_SYS_PSEUDO, SYS_MEMCPY_PSEUDO,
            AVM_SYS_INTRINSIC, avm_memcpy, AVM_NO_COST, AVM_NO_COST)
// memcpy_p is selected from the generic cross-address-space memcpy intrinsic,
// so it deliberately has no target-specific LLVM intrinsic identity.
AVM_SYS_DEF(0x10, memcpy_p, AVM_SYS_PSEUDO, SYS_MEMCPY_P_PSEUDO,
            AVM_NO_INTRINSIC, AVM_NO_INTRINSIC, AVM_NO_COST, AVM_NO_COST)
AVM_SYS_DEF(0x11, memset, AVM_SYS_PSEUDO, SYS_MEMSET_PSEUDO,
            AVM_SYS_INTRINSIC, avm_memset, AVM_NO_COST, AVM_NO_COST)
AVM_SYS_DEF(0x12, memmove, AVM_SYS_PSEUDO, SYS_MEMMOVE_PSEUDO,
            AVM_SYS_INTRINSIC, avm_memmove, AVM_NO_COST, AVM_NO_COST)
AVM_SYS_DEF(0x13, memcmp_p, AVM_SYS_PSEUDO, SYS_MEMCMP_P_PSEUDO,
            AVM_SYS_INTRINSIC, avm_memcmp_p, AVM_NO_COST, AVM_NO_COST)
AVM_SYS_DEF(0x14, strcmp_p, AVM_SYS_PSEUDO, SYS_STRCMP_P_PSEUDO,
            AVM_SYS_INTRINSIC, avm_strcmp_p, AVM_NO_COST, AVM_NO_COST)
AVM_SYS_DEF(0x15, strlen_p, AVM_SYS_PSEUDO, SYS_STRLEN_P_PSEUDO,
            AVM_SYS_INTRINSIC, avm_strlen_p, AVM_NO_COST, AVM_NO_COST)
AVM_SYS_DEF(0x16, strncpy_p, AVM_SYS_PSEUDO, SYS_STRNCPY_P_PSEUDO,
            AVM_SYS_INTRINSIC, avm_strncpy_p, AVM_NO_COST, AVM_NO_COST)
AVM_SYS_DEF(0x17, strncat_p, AVM_SYS_PSEUDO, SYS_STRNCAT_P_PSEUDO,
            AVM_SYS_INTRINSIC, avm_strncat_p, AVM_NO_COST, AVM_NO_COST)

AVM_SYS_DEF(0x18, memcmp, AVM_SYS_PSEUDO, SYS_MEMCMP_PSEUDO,
            AVM_SYS_INTRINSIC, avm_memcmp, AVM_NO_COST, AVM_NO_COST)
AVM_SYS_DEF(0x19, strcmp, AVM_SYS_PSEUDO, SYS_STRCMP_PSEUDO,
            AVM_SYS_INTRINSIC, avm_strcmp, AVM_NO_COST, AVM_NO_COST)
AVM_SYS_DEF(0x1a, strlen, AVM_SYS_PSEUDO, SYS_STRLEN_PSEUDO,
            AVM_SYS_INTRINSIC, avm_strlen, AVM_NO_COST, AVM_NO_COST)
AVM_SYS_DEF(0x1b, strncpy, AVM_SYS_PSEUDO, SYS_STRNCPY_PSEUDO,
            AVM_SYS_INTRINSIC, avm_strncpy, AVM_NO_COST, AVM_NO_COST)
AVM_SYS_DEF(0x1c, strncat, AVM_SYS_PSEUDO, SYS_STRNCAT_PSEUDO,
            AVM_SYS_INTRINSIC, avm_strncat, AVM_NO_COST, AVM_NO_COST)

AVM_SYS_DEF(0x1d, display, AVM_SYS_PSEUDO, SYS_DISPLAY_PSEUDO,
            AVM_SYS_INTRINSIC, avm_display, AVM_NO_COST, AVM_NO_COST)

AVM_SYS_DEF(0x1e, draw_sprite_overwrite, AVM_SYS_PSEUDO,
            SYS_DRAW_SPRITE_OVERWRITE_PSEUDO, AVM_SYS_INTRINSIC,
            avm_draw_sprite_overwrite, AVM_NO_COST, AVM_NO_COST)
AVM_SYS_DEF(0x1f, draw_sprite_plus_mask, AVM_SYS_PSEUDO,
            SYS_DRAW_SPRITE_PLUS_MASK_PSEUDO, AVM_SYS_INTRINSIC,
            avm_draw_sprite_plus_mask, AVM_NO_COST, AVM_NO_COST)
AVM_SYS_DEF(0x20, draw_sprite_self_masked, AVM_SYS_PSEUDO,
            SYS_DRAW_SPRITE_SELF_MASKED_PSEUDO, AVM_SYS_INTRINSIC,
            avm_draw_sprite_self_masked, AVM_NO_COST, AVM_NO_COST)
AVM_SYS_DEF(0x21, draw_sprite_erase, AVM_SYS_PSEUDO,
            SYS_DRAW_SPRITE_ERASE_PSEUDO, AVM_SYS_INTRINSIC,
            avm_draw_sprite_erase, AVM_NO_COST, AVM_NO_COST)

AVM_SYS_DEF(0x22, set_sprite, AVM_SYS_PSEUDO,
            SYS_SET_SPRITE_PSEUDO, AVM_SYS_INTRINSIC,
            avm_set_sprite, AVM_NO_COST, AVM_NO_COST)

AVM_SYS_DEF(0x23, draw_overwrite, AVM_SYS_PSEUDO,
            SYS_DRAW_OVERWRITE_PSEUDO, AVM_SYS_INTRINSIC,
            avm_draw_overwrite, AVM_NO_COST, AVM_NO_COST)
AVM_SYS_DEF(0x24, draw_plus_mask, AVM_SYS_PSEUDO,
            SYS_DRAW_PLUS_MASK_PSEUDO, AVM_SYS_INTRINSIC,
            avm_draw_plus_mask, AVM_NO_COST, AVM_NO_COST)
AVM_SYS_DEF(0x25, draw_self_masked, AVM_SYS_PSEUDO,
            SYS_DRAW_SELF_MASKED_PSEUDO, AVM_SYS_INTRINSIC,
            avm_draw_self_masked, AVM_NO_COST, AVM_NO_COST)
AVM_SYS_DEF(0x26, draw_erase, AVM_SYS_PSEUDO,
            SYS_DRAW_ERASE_PSEUDO, AVM_SYS_INTRINSIC,
            avm_draw_erase, AVM_NO_COST, AVM_NO_COST)

AVM_SYS_DEF(0x27, draw_filled_rect_white, AVM_SYS_PSEUDO,
            SYS_DRAW_FILLED_RECT_WHITE_PSEUDO, AVM_SYS_INTRINSIC,
            avm_draw_filled_rect_white, AVM_NO_COST, AVM_NO_COST)
AVM_SYS_DEF(0x28, draw_filled_rect_black, AVM_SYS_PSEUDO,
            SYS_DRAW_FILLED_RECT_BLACK_PSEUDO, AVM_SYS_INTRINSIC,
            avm_draw_filled_rect_black, AVM_NO_COST, AVM_NO_COST)

AVM_SYS_DEF(0x29, buttons, AVM_SYS_PSEUDO, SYS_BUTTONS_PSEUDO,
            AVM_SYS_INTRINSIC, avm_buttons, AVM_NO_COST, AVM_NO_COST)
AVM_SYS_DEF(0x2a, idle, AVM_SYS_PSEUDO, SYS_IDLE_PSEUDO,
            AVM_SYS_INTRINSIC, avm_idle, AVM_NO_COST, AVM_NO_COST)
AVM_SYS_DEF(0x2b, generate_random_seed, AVM_SYS_PSEUDO,
            SYS_GENERATE_RANDOM_SEED_PSEUDO, AVM_SYS_INTRINSIC,
            avm_generate_random_seed, AVM_NO_COST, AVM_NO_COST)
AVM_SYS_DEF(0x2c, save, AVM_SYS_PSEUDO, SYS_SAVE_PSEUDO,
            AVM_SYS_INTRINSIC, avm_save, AVM_NO_COST, AVM_NO_COST)
AVM_SYS_DEF(0x2d, load, AVM_SYS_PSEUDO, SYS_LOAD_PSEUDO,
            AVM_SYS_INTRINSIC, avm_load, AVM_NO_COST, AVM_NO_COST)
AVM_SYS_DEF(0x2e, save_exists, AVM_SYS_PSEUDO, SYS_SAVE_EXISTS_PSEUDO,
            AVM_SYS_INTRINSIC, avm_save_exists, AVM_NO_COST, AVM_NO_COST)

AVM_SYS_DEF(0x2f, vsnprintf, AVM_SYS_PSEUDO, SYS_VSNPRINTF_PSEUDO,
            AVM_SYS_INTRINSIC, avm_vsnprintf, AVM_NO_COST, AVM_NO_COST)
AVM_SYS_DEF(0x30, vsnprintf_p, AVM_SYS_PSEUDO, SYS_VSNPRINTF_P_PSEUDO,
            AVM_SYS_INTRINSIC, avm_vsnprintf_p, AVM_NO_COST, AVM_NO_COST)

#ifdef AVM_SYS_DEF
#undef AVM_SYS_DEF
#endif
