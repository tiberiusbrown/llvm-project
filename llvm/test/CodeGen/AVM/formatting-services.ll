; RUN: llc -mtriple=avm -O0 -verify-machineinstrs < %s | FileCheck %s --check-prefix=ASM
; RUN: llc -mtriple=avm -O2 -verify-machineinstrs < %s | FileCheck %s --check-prefix=ASM
; RUN: llc -mtriple=avm -O0 -stop-after=avm-system-service-regions < %s -o - \
; RUN:   | FileCheck %s --check-prefix=MIR

@ram_format = internal constant [3 x i8] c"%u\00", align 1
@program_format = internal addrspace(1) constant [3 x i8] c"%S\00", align 1

declare i16 @llvm.avm.vsnprintf(ptr, i16, ptr, ptr)
declare i16 @llvm.avm.vsnprintf.p(ptr, i16, ptr addrspace(1), ptr)

define i16 @format_ram(ptr %dst, i16 %size, ptr %ap) {
; ASM-LABEL: format_ram:
; ASM:       sys vsnprintf
; MIR-LABEL: name: format_ram
; MIR:       SYS_VSNPRINTF_PSEUDO
  %result = call i16 @llvm.avm.vsnprintf(
      ptr %dst, i16 %size, ptr @ram_format, ptr %ap)
  ret i16 %result
}

define i16 @format_program(ptr %dst, i16 %size, ptr %ap) {
; ASM-LABEL: format_program:
; ASM:       sys vsnprintf_p
; MIR-LABEL: name: format_program
; MIR:       SYS_VSNPRINTF_P_PSEUDO
  %result = call i16 @llvm.avm.vsnprintf.p(
      ptr %dst, i16 %size, ptr addrspace(1) @program_format, ptr %ap)
  ret i16 %result
}

define i16 @format_program_unnormalized(ptr %dst, i16 %size, ptr %ap,
                                        i32 %bits) {
; ASM-LABEL: format_program_unnormalized:
; ASM:       sys vsnprintf_p
; MIR-LABEL: name: format_program_unnormalized
; MIR-NOT:   PROG_CANON_PSEUDO
; MIR:       SYS_VSNPRINTF_P_PSEUDO
  %format = inttoptr i32 %bits to ptr addrspace(1)
  %result = call i16 @llvm.avm.vsnprintf.p(
      ptr %dst, i16 %size, ptr addrspace(1) %format, ptr %ap)
  ret i16 %result
}

; The semantic pseudos carry explicit output/input values and memory references;
; the descriptor-driven post-RA expander assigns the service ABI registers:
;
;   r4  destination/result
;   r5  size
;   r6  RAM format, or q3 for a program format
;   r2  va_list cursor
;
; Both services also carry conservative unknown AS0/AS1 reads for %s/%S.
