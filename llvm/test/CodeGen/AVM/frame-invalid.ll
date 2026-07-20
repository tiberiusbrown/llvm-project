; RUN: split-file %s %t
; RUN: not --crash llc -mtriple=avm-unknown-arduboyfx -mcpu=avm1 \
; RUN:   -mtune=avm-interpreter-32u4-v1 -O0 %t/large.ll -o /dev/null 2>&1 \
; RUN:   | FileCheck %s --check-prefix=LARGE
; RUN: not --crash llc -mtriple=avm-unknown-arduboyfx -mcpu=avm1 \
; RUN:   -mtune=avm-interpreter-32u4-v1 -O0 %t/dynamic.ll -o /dev/null 2>&1 \
; RUN:   | FileCheck %s --check-prefix=DYNAMIC

; LARGE: LLVM ERROR: AVM fixed frame exceeds 256 bytes
; DYNAMIC: LLVM ERROR: dynamic AVM stack allocation is unsupported

;--- large.ll
define void @large_frame(i16 %x) {
  %a = alloca [257 x i8], align 1
  %p = getelementptr [257 x i8], ptr %a, i16 0, i16 256
  store volatile i8 0, ptr %p, align 1
  ret void
}

;--- dynamic.ll
define void @dynamic_frame(i16 %size) {
  %a = alloca i8, i16 %size, align 1
  store volatile i8 0, ptr %a, align 1
  ret void
}
