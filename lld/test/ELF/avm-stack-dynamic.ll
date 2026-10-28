; RUN: llc -mtriple=avm -O0 -verify-machineinstrs -filetype=obj %s -o %t.o
; RUN: ld.lld -e dynamic --gc-sections --avm-print-stack-usage --avm-print-stack-gaps %t.o -o %t.out 2>&1 | FileCheck %s --check-prefix=DYNAMIC
; RUN: not ld.lld -e overflow --gc-sections %t.o -o %t.overflow 2>&1 | FileCheck %s --check-prefix=OVERFLOW
; RUN: llvm-as %s -o %t.bc
; RUN: ld.lld -e dynamic --gc-sections --lto-O2 --avm-print-stack-usage --avm-print-stack-gaps %t.bc -o %t.lto 2>&1 | FileCheck %s --check-prefix=DYNAMIC
; RUN: not ld.lld -e overflow --gc-sections --lto-O2 %t.bc -o %t.overflow-lto 2>&1 | FileCheck %s --check-prefix=OVERFLOW

; DYNAMIC: complete bound: no
; DYNAMIC: dynamic: frame 36
; DYNAMIC: dynamic: compiler-marked incomplete
; OVERFLOW: error: AVM maximum provable stack usage is {{[0-9]+}} bytes; limit is 256 bytes

target triple = "avm"
target datalayout = "e-P1-p:16:8-p1:24:8-i8:8-i16:8-i24:8-i32:8-i64:8-f32:8-f64:8-a:8-n8:16-S8"

; Unknown dynamic bytes must not hide the known fixed contribution.
define void @dynamic(i16 %n) addrspace(1) noinline {
  %fixed = alloca [32 x i8], align 1
  store volatile i8 42, ptr %fixed
  call void asm sideeffect "", "r,~{memory}"(ptr %fixed)
  %a = alloca i8, i16 %n, align 1
  store volatile i8 17, ptr %a
  %v = load volatile i8, ptr %fixed
  ret void
}

define void @overflow(i16 %n) addrspace(1) {
  %a = alloca [240 x i8], align 1
  store volatile i8 1, ptr %a
  call void asm sideeffect "", "r,~{memory}"(ptr %a)
  call addrspace(1) void @dynamic(i16 %n)
  call void asm sideeffect "", "r,~{memory}"(ptr %a)
  %v = load volatile i8, ptr %a
  ret void
}
