; RUN: split-file %s %t
; RUN: llvm-as %t/safe.ll -o %t/safe.bc
; RUN: ld.lld -O2 --gc-sections --avm-print-stack-usage %t/safe.bc -o %t/safe 2>&1 | FileCheck %s --check-prefix=SAFE
; RUN: llvm-readobj --sections --symbols %t/safe | FileCheck %s --check-prefix=STRIP
; RUN: llvm-as %t/over.ll -o %t/over.bc
; RUN: not ld.lld -O2 --gc-sections %t/over.bc -o %t/over 2>&1 | FileCheck %s --check-prefix=OVER
; RUN: opt -passes='default<O2>' %t/safe.bc -o %t/optimized.bc
; RUN: llc -O2 -filetype=obj %t/optimized.bc -o %t/safe.o
; RUN: ld.lld --gc-sections --avm-print-stack-usage %t/safe.o -o %t/nolto 2>&1 | FileCheck %s --check-prefix=SAFE

; LTO resolves a constant function pointer, retains noinline calls, removes an
; unused oversized frame before codegen, and inlines a trivial helper. Both
; ordinary and LTO links consume the same finalized machine metadata.
; SAFE: AVM maximum provable stack usage is 156 bytes; complete bound: yes
; SAFE: _start: frame 40
; SAFE: middle: frame 50
; SAFE: relay: frame 0
; SAFE: tail transfer: return address 0, outgoing arguments 0
; SAFE: leaf: frame 60
; STRIP-NOT: Name: .stack_sizes
; STRIP-NOT: Name: .avm.stackcalls
; STRIP-NOT: Name: gone
; STRIP-NOT: Name: inlined
; STRIP: Name: _start
; STRIP-NOT: Name: .stack_sizes
; STRIP-NOT: Name: .avm.stackcalls
; STRIP-NOT: Name: gone
; STRIP-NOT: Name: inlined
; OVER: error: AVM maximum provable stack usage is 286 bytes; limit is 256 bytes
; OVER: _start: frame 100
; OVER: middle: frame 100
; OVER: leaf: frame 80

;--- safe.ll
target datalayout = "e-m:e-p:16:8-p1:24:8-i8:8-i16:8-i32:8-i64:8-f16:8-f32:8-n8:16-S8-P1-G0-A0"
target triple = "avm-unknown-arduboyfx"
@saved = internal global ptr null
@fp = internal constant ptr addrspace(1) @relay
define void @_start() addrspace(1) {
  %a = alloca [40 x i8], align 1
  store volatile ptr %a, ptr @saved
  %p = getelementptr [40 x i8], ptr %a, i16 0, i16 39
  store volatile i8 1, ptr %p
  call addrspace(1) void @middle()
  ret void
}
define internal void @middle() addrspace(1) noinline {
  %a = alloca [50 x i8], align 1
  store volatile ptr %a, ptr @saved
  %p = getelementptr [50 x i8], ptr %a, i16 0, i16 49
  store volatile i8 1, ptr %p
  %f = load ptr addrspace(1), ptr @fp
  call addrspace(1) void %f()
  ret void
}
define internal void @leaf() addrspace(1) noinline {
  %a = alloca [60 x i8], align 1
  store volatile ptr %a, ptr @saved
  %p = getelementptr [60 x i8], ptr %a, i16 0, i16 59
  %x = call addrspace(1) i8 @inlined()
  store volatile i8 %x, ptr %p
  ret void
}
define internal void @relay() addrspace(1) noinline {
  call addrspace(1) void @leaf()
  ret void
}
define internal i8 @inlined() addrspace(1) alwaysinline { ret i8 1 }
define internal void @gone() addrspace(1) noinline {
  %a = alloca [300 x i8], align 1
  %p = getelementptr [300 x i8], ptr %a, i16 0, i16 299
  store volatile i8 1, ptr %p
  ret void
}

;--- over.ll
target datalayout = "e-m:e-p:16:8-p1:24:8-i8:8-i16:8-i32:8-i64:8-f16:8-f32:8-n8:16-S8-P1-G0-A0"
target triple = "avm-unknown-arduboyfx"
@saved = internal global ptr null
define void @_start() addrspace(1) {
  %a = alloca [100 x i8], align 1
  store volatile ptr %a, ptr @saved
  %p = getelementptr [100 x i8], ptr %a, i16 0, i16 99
  store volatile i8 1, ptr %p
  call addrspace(1) void @middle()
  ret void
}
define internal void @middle() addrspace(1) noinline {
  %a = alloca [100 x i8], align 1
  store volatile ptr %a, ptr @saved
  %p = getelementptr [100 x i8], ptr %a, i16 0, i16 99
  store volatile i8 1, ptr %p
  call addrspace(1) void @relay()
  ret void
}
define internal void @leaf() addrspace(1) noinline {
  %a = alloca [80 x i8], align 1
  store volatile ptr %a, ptr @saved
  %p = getelementptr [80 x i8], ptr %a, i16 0, i16 79
  store volatile i8 1, ptr %p
  ret void
}
define internal void @relay() addrspace(1) noinline {
  call addrspace(1) void @leaf()
  ret void
}
