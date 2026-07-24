; RUN: llc -mtriple=avm -O0 --frame-pointer=none -verify-machineinstrs < %s | FileCheck %s --check-prefix=ASM
; RUN: llc -mtriple=avm -O2 --frame-pointer=none -verify-machineinstrs < %s | FileCheck %s --check-prefix=ASM
; RUN: llc -mtriple=avm -O2 -stop-after=avm-system-service-regions < %s -o - | FileCheck %s --check-prefix=MIR
; RUN: llc -mtriple=avm -O2 -filetype=obj -verify-machineinstrs < %s -o %t.o

declare i16 @llvm.avm.memcmp.p(ptr, ptr addrspace(1), i16)
declare i16 @llvm.avm.strcmp.p(ptr, ptr addrspace(1))
declare i16 @llvm.avm.strlen.p(ptr addrspace(1))
declare ptr @llvm.avm.strncpy.p(ptr, ptr addrspace(1), i16)
declare ptr @llvm.avm.strncat.p(ptr, ptr addrspace(1), i16)
declare i16 @llvm.avm.memcmp(ptr, ptr, i16)
declare i16 @llvm.avm.strcmp(ptr, ptr)
declare i16 @llvm.avm.strlen(ptr)
declare ptr @llvm.avm.strncpy(ptr, ptr, i16)
declare ptr @llvm.avm.strncat(ptr, ptr, i16)
declare void @llvm.memcpy.p0.p1.i16(ptr, ptr addrspace(1), i16, i1 immarg)

define i16 @test_memcmp_p(ptr %lhs, ptr addrspace(1) %rhs, i16 %n) {
; ASM-LABEL: test_memcmp_p:
; ASM: sys memcmp_p
; MIR-LABEL: name: test_memcmp_p
; MIR: :gpr32 = PROG_CANON_PSEUDO
; MIR: :gpr16 = SYS_MEMCMP_P_PSEUDO {{%[0-9]+}}, {{(killed )?}}{{%[0-9]+}}, {{(killed )?}}{{%[0-9]+}}
; MIR-SAME: :: (load unknown-size, align 1), (load unknown-size, align 1, addrspace 1)
  %r = call i16 @llvm.avm.memcmp.p(ptr %lhs, ptr addrspace(1) %rhs, i16 %n)
  ret i16 %r
}

define i16 @test_strcmp_p(ptr %lhs, ptr addrspace(1) %rhs) {
; ASM-LABEL: test_strcmp_p:
; ASM: sys strcmp_p
; MIR-LABEL: name: test_strcmp_p
; MIR: :gpr32 = PROG_CANON_PSEUDO
; MIR: :gpr16 = SYS_STRCMP_P_PSEUDO
  %r = call i16 @llvm.avm.strcmp.p(ptr %lhs, ptr addrspace(1) %rhs)
  ret i16 %r
}

define i16 @test_strlen_p(ptr addrspace(1) %src) {
; ASM-LABEL: test_strlen_p:
; ASM: sys strlen_p
; MIR-LABEL: name: test_strlen_p
; MIR: :gpr32 = PROG_CANON_PSEUDO
; MIR: :gpr16 = SYS_STRLEN_P_PSEUDO
; MIR-SAME: :: (load unknown-size, align 1, addrspace 1)
  %r = call i16 @llvm.avm.strlen.p(ptr addrspace(1) %src)
  ret i16 %r
}

define ptr @test_strncpy_p(ptr %dst, ptr addrspace(1) %src, i16 %n) {
; ASM-LABEL: test_strncpy_p:
; ASM: sys strncpy_p
; MIR-LABEL: name: test_strncpy_p
; MIR: :gpr32 = PROG_CANON_PSEUDO
; MIR: :gpr16 = SYS_STRNCPY_P_PSEUDO {{%[0-9]+}}, {{(killed )?}}{{%[0-9]+}}, {{(killed )?}}{{%[0-9]+}}
; MIR-SAME: :: (store unknown-size, align 1), (load unknown-size, align 1, addrspace 1)
  %r = call ptr @llvm.avm.strncpy.p(ptr %dst, ptr addrspace(1) %src, i16 %n)
  ret ptr %r
}

define ptr @test_strncat_p(ptr %dst, ptr addrspace(1) %src, i16 %n) {
; ASM-LABEL: test_strncat_p:
; ASM: sys strncat_p
; MIR-LABEL: name: test_strncat_p
; MIR: :gpr32 = PROG_CANON_PSEUDO
; MIR: :gpr16 = SYS_STRNCAT_P_PSEUDO {{%[0-9]+}}, {{(killed )?}}{{%[0-9]+}}, {{(killed )?}}{{%[0-9]+}}
; MIR-SAME: :: (load unknown-size, align 1), (store unknown-size, align 1), (load unknown-size, align 1, addrspace 1)
  %r = call ptr @llvm.avm.strncat.p(ptr %dst, ptr addrspace(1) %src, i16 %n)
  ret ptr %r
}

define i16 @test_memcmp(ptr %lhs, ptr %rhs, i16 %n) {
; ASM-LABEL: test_memcmp:
; ASM: sys memcmp
; MIR-LABEL: name: test_memcmp
; MIR: :gpr16 = SYS_MEMCMP_PSEUDO
; MIR-SAME: :: (load unknown-size, align 1), (load unknown-size, align 1)
  %r = call i16 @llvm.avm.memcmp(ptr %lhs, ptr %rhs, i16 %n)
  ret i16 %r
}

define i16 @test_strcmp(ptr %lhs, ptr %rhs) {
; ASM-LABEL: test_strcmp:
; ASM: sys strcmp
; MIR-LABEL: name: test_strcmp
; MIR: :gpr16 = SYS_STRCMP_PSEUDO
  %r = call i16 @llvm.avm.strcmp(ptr %lhs, ptr %rhs)
  ret i16 %r
}

define i16 @test_strlen(ptr %src) {
; ASM-LABEL: test_strlen:
; ASM: sys strlen
; MIR-LABEL: name: test_strlen
; MIR: :gpr16 = SYS_STRLEN_PSEUDO
; MIR-SAME: :: (load unknown-size, align 1)
  %r = call i16 @llvm.avm.strlen(ptr %src)
  ret i16 %r
}

define ptr @test_strncpy(ptr %dst, ptr %src, i16 %n) {
; ASM-LABEL: test_strncpy:
; ASM: sys strncpy
; MIR-LABEL: name: test_strncpy
; MIR: :gpr16 = SYS_STRNCPY_PSEUDO
; MIR-SAME: :: (store unknown-size, align 1), (load unknown-size, align 1)
  %r = call ptr @llvm.avm.strncpy(ptr %dst, ptr %src, i16 %n)
  ret ptr %r
}

define ptr @test_strncat(ptr %dst, ptr %src, i16 %n) {
; ASM-LABEL: test_strncat:
; ASM: sys strncat
; MIR-LABEL: name: test_strncat
; MIR: :gpr16 = SYS_STRNCAT_PSEUDO
; MIR-SAME: :: (load unknown-size, align 1), (store unknown-size, align 1), (load unknown-size, align 1)
  %r = call ptr @llvm.avm.strncat(ptr %dst, ptr %src, i16 %n)
  ret ptr %r
}

define i16 @constant_memcmp(ptr %lhs, ptr %rhs) {
; MIR-LABEL: name: constant_memcmp
; MIR: SYS_MEMCMP_PSEUDO
; MIR-SAME: :: (load (s64), align 1), (load (s64), align 1)
  %r = call i16 @llvm.avm.memcmp(ptr %lhs, ptr %rhs, i16 8)
  ret i16 %r
}

define ptr @constant_strncpy(ptr %dst, ptr %src) {
; MIR-LABEL: name: constant_strncpy
; MIR: SYS_STRNCPY_PSEUDO
; MIR-SAME: :: (store (s56), align 1), (load (s56), align 1)
  %r = call ptr @llvm.avm.strncpy(ptr %dst, ptr %src, i16 7)
  ret ptr %r
}

define i16 @inttoptr_program_services(ptr %dst, ptr %lhs, i32 %bits, i16 %n) {
; MIR-LABEL: name: inttoptr_program_services
; MIR: PROG_CANON_PSEUDO
; MIR: :q3only = nomerge COPY
; MIR: SYS_MEMCMP_P_PSEUDO
; MIR: SYS_STRCMP_P_PSEUDO
; MIR: SYS_STRLEN_P_PSEUDO
; MIR: SYS_STRNCPY_P_PSEUDO
; MIR: SYS_STRNCAT_P_PSEUDO
  %src = inttoptr i32 %bits to ptr addrspace(1)
  %a = call i16 @llvm.avm.memcmp.p(ptr %lhs, ptr addrspace(1) %src, i16 %n)
  %b = call i16 @llvm.avm.strcmp.p(ptr %lhs, ptr addrspace(1) %src)
  %c = call i16 @llvm.avm.strlen.p(ptr addrspace(1) %src)
  %d = call ptr @llvm.avm.strncpy.p(ptr %dst, ptr addrspace(1) %src, i16 %n)
  %e = call ptr @llvm.avm.strncat.p(ptr %d, ptr addrspace(1) %src, i16 %n)
  %ab = add i16 %a, %b
  %abc = add i16 %ab, %c
  ret i16 %abc
}

define void @generic_memcpy_p_unnormalized(ptr %dst, i32 %bits, i16 %n) {
; MIR-LABEL: name: generic_memcpy_p_unnormalized
; MIR-NOT: PROG_CANON_PSEUDO
; MIR: SYS_MEMCPY_P_PSEUDO
  %src = inttoptr i32 %bits to ptr addrspace(1)
  call void @llvm.memcpy.p0.p1.i16(ptr %dst, ptr addrspace(1) %src, i16 %n,
                                   i1 false)
  ret void
}

define i16 @result_and_inputs_live(ptr %dst, ptr %lhs, ptr %rhs, i16 %n) {
  %cmp = call i16 @llvm.avm.memcmp(ptr %lhs, ptr %rhs, i16 %n)
  %len = call i16 @llvm.avm.strlen(ptr %dst)
  %copy = call ptr @llvm.avm.strncpy(ptr %dst, ptr %rhs, i16 %n)
  %cat = call ptr @llvm.avm.strncat(ptr %copy, ptr %lhs, i16 %n)
  %di = ptrtoint ptr %dst to i16
  %li = ptrtoint ptr %lhs to i16
  %ri = ptrtoint ptr %rhs to i16
  %x0 = add i16 %cmp, %len
  %x1 = add i16 %x0, %di
  %x2 = add i16 %x1, %li
  %x3 = add i16 %x2, %ri
  %x4 = add i16 %x3, %n
  ret i16 %x4
}
