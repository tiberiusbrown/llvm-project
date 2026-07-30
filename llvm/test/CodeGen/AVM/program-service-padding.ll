; RUN: llc -mtriple=avm -O2 --frame-pointer=none -verify-machineinstrs < %s | FileCheck %s --check-prefix=ASM
; RUN: llc -mtriple=avm -O2 -stop-after=avm-system-service-regions < %s -o - | FileCheck %s --check-prefix=MIR

declare i16 @llvm.avm.memcmp.p(ptr, ptr addrspace(1), i16)
declare i16 @llvm.avm.strcmp.p(ptr, ptr addrspace(1))
declare i16 @llvm.avm.strlen.p(ptr addrspace(1))
declare ptr @llvm.avm.strncpy.p(ptr, ptr addrspace(1), i16)
declare ptr @llvm.avm.strncat.p(ptr, ptr addrspace(1), i16)

define i16 @memcmp_p_ignores_padding(ptr %lhs, i32 %bits, i16 %n) {
; ASM-LABEL: memcmp_p_ignores_padding:
; ASM:       sys memcmp_p
; MIR-LABEL: name: memcmp_p_ignores_padding
; MIR-NOT:   PROG_CANON_PSEUDO
; MIR:       SYS_MEMCMP_P_PSEUDO
  %rhs = inttoptr i32 %bits to ptr addrspace(1)
  %result = call i16 @llvm.avm.memcmp.p(
      ptr %lhs, ptr addrspace(1) %rhs, i16 %n)
  ret i16 %result
}

define i16 @strcmp_p_ignores_padding(ptr %lhs, i32 %bits) {
; ASM-LABEL: strcmp_p_ignores_padding:
; ASM:       sys strcmp_p
; MIR-LABEL: name: strcmp_p_ignores_padding
; MIR-NOT:   PROG_CANON_PSEUDO
; MIR:       SYS_STRCMP_P_PSEUDO
  %rhs = inttoptr i32 %bits to ptr addrspace(1)
  %result = call i16 @llvm.avm.strcmp.p(
      ptr %lhs, ptr addrspace(1) %rhs)
  ret i16 %result
}

define i16 @strlen_p_ignores_padding(i32 %bits) {
; ASM-LABEL: strlen_p_ignores_padding:
; ASM:       sys strlen_p
; MIR-LABEL: name: strlen_p_ignores_padding
; MIR-NOT:   PROG_CANON_PSEUDO
; MIR:       SYS_STRLEN_P_PSEUDO
  %src = inttoptr i32 %bits to ptr addrspace(1)
  %result = call i16 @llvm.avm.strlen.p(ptr addrspace(1) %src)
  ret i16 %result
}

define ptr @strncpy_p_ignores_padding(ptr %dst, i32 %bits, i16 %n) {
; ASM-LABEL: strncpy_p_ignores_padding:
; ASM:       sys strncpy_p
; MIR-LABEL: name: strncpy_p_ignores_padding
; MIR-NOT:   PROG_CANON_PSEUDO
; MIR:       SYS_STRNCPY_P_PSEUDO
  %src = inttoptr i32 %bits to ptr addrspace(1)
  %result = call ptr @llvm.avm.strncpy.p(
      ptr %dst, ptr addrspace(1) %src, i16 %n)
  ret ptr %result
}

define ptr @strncat_p_ignores_padding(ptr %dst, i32 %bits, i16 %n) {
; ASM-LABEL: strncat_p_ignores_padding:
; ASM:       sys strncat_p
; MIR-LABEL: name: strncat_p_ignores_padding
; MIR-NOT:   PROG_CANON_PSEUDO
; MIR:       SYS_STRNCAT_P_PSEUDO
  %src = inttoptr i32 %bits to ptr addrspace(1)
  %result = call ptr @llvm.avm.strncat.p(
      ptr %dst, ptr addrspace(1) %src, i16 %n)
  ret ptr %result
}
