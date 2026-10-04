; RUN: llc -mtriple=avm -O0 -filetype=obj %s -o %t.o
; RUN: ld.lld --gc-sections --avm-print-stack-usage --avm-print-stack-gaps %t.o -o %t.out 2>&1 | FileCheck %s --check-prefix=SAFE
; RUN: ld.lld -r %t.o -o %t.partial
; RUN: ld.lld --gc-sections --avm-print-stack-usage --avm-print-stack-gaps %t.partial -o %t.relinked 2>&1 | FileCheck %s --check-prefix=SAFE
; RUN: ld.lld -e opaque --avm-print-stack-usage --avm-print-stack-gaps %t.o -o %t.opaque 2>&1 | FileCheck %s --check-prefix=OPAQUE
; RUN: llvm-as %s -o %t.bc
; RUN: ld.lld --gc-sections --lto-O2 --avm-print-stack-usage --avm-print-stack-gaps %t.bc -o %t.lto 2>&1 | FileCheck %s --check-prefix=SAFE
; RUN: ld.lld -e opaque --lto-O2 --avm-print-stack-usage --avm-print-stack-gaps %t.bc -o %t.lto-opaque 2>&1 | FileCheck %s --check-prefix=OPAQUE

; SAFE: complete bound: yes
; SAFE: safe: frame {{[3-9][0-9]}}
; SAFE: AVM stack analysis gaps: 0; paths shown: 0; omitted: 0
; OPAQUE: complete bound: no
; OPAQUE: opaque: compiler-marked incomplete
; OPAQUE: AVM stack analysis gaps: 1; paths shown: 1; omitted: 0

target triple = "avm"
target datalayout = "e-P1-p:16:8-p1:24:8-i8:8-i16:8-i24:8-i32:8-i64:8-f32:8-f64:8-a:8-n8:16-S8"
@output = global i16 0, align 1

define void @_start() addrspace(1) {
  %result = call addrspace(1) i16 @safe(i16 3, i16 7)
  store volatile i16 %result, ptr @output
  ret void
}

define i16 @safe(i16 %a, i16 %b) addrspace(1) noinline {
  %slot = alloca [32 x i8], align 1
  store volatile i8 1, ptr %slot
  %result = call i16 asm "mul16 $0, $2", "=r,0,r"(i16 %a, i16 %b)
  call void asm sideeffect "", "r,~{memory}"(ptr %slot)
  ret i16 %result
}

define void @opaque() addrspace(1) {
  call void asm sideeffect "push16 r4\0Apop16 r4", ""()
  ret void
}
