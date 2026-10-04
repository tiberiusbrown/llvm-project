; RUN: llc -mtriple=avm -O0 -function-sections -filetype=obj %s -o %t.o
; RUN: ld.lld --avm-print-stack-usage --avm-print-stack-gaps %t.o -o %t.out 2>&1 | FileCheck %s --check-prefix=SAFE
; RUN: ld.lld -r %t.o -o %t.partial
; RUN: llvm-readobj --sections %t.partial | FileCheck %s --check-prefix=KEEP
; RUN: ld.lld --avm-print-stack-usage --avm-print-stack-gaps %t.partial -o %t.relinked 2>&1 | FileCheck %s --check-prefix=SAFE
; RUN: not ld.lld -e overflow %t.o -o %t.over 2>&1 | FileCheck %s --check-prefix=OVER
; RUN: ld.lld -e unguarded --avm-print-stack-gaps %t.o -o %t.weak 2>&1 | FileCheck %s --check-prefix=WEAK
; RUN: ld.lld -e unknown_registration --avm-print-stack-gaps %t.o -o %t.unknown 2>&1 | FileCheck %s --check-prefix=UNKNOWN
; RUN: ld.lld --gc-sections -e known_registration --avm-print-stack-usage --avm-print-stack-gaps %t.o -o %t.registry 2>&1 | FileCheck %s --check-prefix=REGISTRY
; RUN: llvm-readobj --sections %t.registry | FileCheck %s --check-prefix=STRIP
; RUN: not ld.lld --gc-sections -e registry_overflow %t.o -o %t.registry-over 2>&1 | FileCheck %s --check-prefix=REGISTRY-OVER
; RUN: ld.lld --defsym=__init_array_start=0x100 --avm-print-stack-gaps %t.o -o %t.bounds 2>&1 | FileCheck %s --check-prefix=UNKNOWN

; SAFE: complete bound: yes
; SAFE: _start: frame
; SAFE: ctor: frame
; SAFE: AVM stack analysis gaps: 0
; OVER: error: AVM maximum provable stack usage is {{[0-9]+}} bytes; limit is 256 bytes
; OVER: overflow: frame
; OVER: ctor: frame
; WEAK: unavailable direct call target
; UNKNOWN: unresolved indirect call target
; REGISTRY: complete bound: yes
; REGISTRY: AVM stack analysis gaps: 0
; REGISTRY-OVER: error: AVM maximum provable stack usage is {{[0-9]+}} bytes; limit is 256 bytes
; REGISTRY-OVER: registered: frame
; KEEP: Name: .avm.stackdtors
; STRIP: Sections [
; STRIP-NOT: .stack_sizes
; STRIP-NOT: .avm.stackcalls
; STRIP-NOT: .avm.stackdtors

target datalayout = "e-P1-p:16:8-p1:24:8-i8:8-i16:8-i24:8-i32:8-i64:8-f32:8-f64:8-a:8-n8:16-S8"
@llvm.global_ctors = appending global [1 x { i32, ptr addrspace(1), ptr }] [{ i32, ptr addrspace(1), ptr } { i32 123, ptr addrspace(1) @ctor, ptr null }]
@llvm.global_dtors = appending global [1 x { i32, ptr addrspace(1), ptr }] [{ i32, ptr addrspace(1), ptr } { i32 456, ptr addrspace(1) @dtor, ptr null }]

define void @_start(ptr addrspace(1) %f) addrspace(1) {
  %buf = alloca [20 x i8], align 1
  store volatile i8 1, ptr %buf
  call addrspace(1) void %f(), !avm.stack.dispatch !0
  call addrspace(1) void %f(), !avm.stack.dispatch !1
  %exists = icmp ne ptr addrspace(1) @weak_hook, null
  br i1 %exists, label %run, label %end
run:
  call addrspace(1) void @weak_hook()
  br label %end
end:
  store volatile i8 2, ptr %buf
  ret void
}
define void @overflow(ptr addrspace(1) %f) addrspace(1) {
  %buf = alloca [180 x i8], align 1
  store volatile i8 1, ptr %buf
  call addrspace(1) void %f(), !avm.stack.dispatch !0
  store volatile i8 2, ptr %buf
  ret void
}
define void @unguarded() addrspace(1) {
  call addrspace(1) void @weak_hook()
  ret void
}
declare extern_weak void @weak_hook() addrspace(1)

define void @ctor() addrspace(1) {
  %buf = alloca [100 x i8], align 1
  store volatile i8 1, ptr %buf
  ret void
}
define void @dtor() addrspace(1) { ret void }
define void @registered() addrspace(1) {
  %buf = alloca [100 x i8], align 1
  store volatile i8 1, ptr %buf
  ret void
}
define void @__avm_register_local_dtor(ptr addrspace(1) %f, ptr %slot) addrspace(1) {
  ret void
}
define void @registry(ptr addrspace(1) %f) addrspace(1) {
  call addrspace(1) void %f(), !avm.stack.dispatch !2
  ret void
}
define void @unknown_registration(ptr addrspace(1) %f) addrspace(1) {
  call addrspace(1) void @__avm_register_local_dtor(ptr addrspace(1) %f, ptr null)
  call addrspace(1) void @registry(ptr addrspace(1) %f)
  ret void
}
define void @known_registration(ptr addrspace(1) %f) addrspace(1) {
  call addrspace(1) void @__avm_register_local_dtor(ptr addrspace(1) @registered, ptr null)
  call addrspace(1) void @registry(ptr addrspace(1) %f)
  ret void
}
define void @registry_overflow(ptr addrspace(1) %f) addrspace(1) {
  %buf = alloca [180 x i8], align 1
  store volatile i8 1, ptr %buf
  call addrspace(1) void @__avm_register_local_dtor(ptr addrspace(1) @registered, ptr null)
  call addrspace(1) void @registry(ptr addrspace(1) %f)
  store volatile i8 2, ptr %buf
  ret void
}
!0 = !{i32 16}
!1 = !{i32 32}
!2 = !{i32 64}
