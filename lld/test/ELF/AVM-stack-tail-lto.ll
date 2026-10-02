; RUN: llvm-as %s -o %t.bc
; RUN: ld.lld -O2 --save-temps --avm-print-stack-usage %t.bc -o %t.lto 2>&1 | FileCheck %s --check-prefix=STACK
; RUN: llvm-objdump -d --no-print-imm-hex %t.lto | FileCheck %s --check-prefix=DIS
; RUN: llc -O0 -verify-machineinstrs -filetype=obj %s -o %t.o
; RUN: ld.lld --avm-print-stack-usage %t.o -o %t.elf 2>&1 | FileCheck %s --check-prefix=STACK
; RUN: llvm-objdump -d --no-print-imm-hex %t.elf | FileCheck %s --check-prefix=DIS

; Releasing a 200-byte frame before entering a 100-byte callee keeps this
; below 256. An ordinary edge would exceed the limit (200 + 3 + 100).
; The callee's frame is deliberately kept intact across IR optimization.
; STACK: AVM maximum provable stack usage is 203 bytes; complete bound: yes
; STACK: _start: frame 0
; STACK: framed: frame 200
; DIS-LABEL: <framed>:
; DIS-NOT: call
; DIS: adjsp 127
; DIS-NEXT: {{.*}}adjsp 73
; DIS-NEXT: {{.*}}jmp{{8|16|f}}
; DIS-NOT: ret
; DIS-LABEL: <leaf>:

target datalayout = "e-m:e-p:16:8-p1:24:8-i8:8-i16:8-i32:8-i64:8-f16:8-f32:8-n8:16-S8-P1-G0-A0"
target triple = "avm-unknown-arduboyfx"
@saved = global ptr null

define void @_start() addrspace(1) {
  notail call addrspace(1) void @framed()
  store volatile ptr null, ptr @saved
  ret void
}
define void @framed() addrspace(1) noinline {
  %a = alloca [200 x i8], align 1
  store volatile ptr %a, ptr @saved
  tail call addrspace(1) void @leaf()
  ret void
}
define void @leaf() addrspace(1) noinline {
  %a = alloca [100 x i8], align 1
  store volatile ptr %a, ptr @saved
  ret void
}
