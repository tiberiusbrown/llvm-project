; RUN: split-file %s %t
; RUN: not llc -mtriple=avm-unknown-arduboyfx -mcpu=avm1 \
; RUN:   -mtune=avm-interpreter-32u4-v1 -O0 %t/large.ll -o /dev/null 2>&1 \
; RUN:   | FileCheck %s --check-prefix=LARGE
; LARGE: error: {{.*}}AVM stack frame size (259) exceeds limit (256)

;--- large.ll
define void @large_frame(i16 %x) {
  %a = alloca [257 x i8], align 1
  %p = getelementptr [257 x i8], ptr %a, i16 0, i16 256
  store volatile i8 0, ptr %p, align 1
  ret void
}
