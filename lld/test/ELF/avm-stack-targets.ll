; RUN: llc -mtriple=avm -O0 -filetype=obj %s -o %t.o
; RUN: ld.lld --avm-print-stack-usage --avm-print-stack-gaps %t.o -o %t.out 2>&1 | FileCheck %s --check-prefix=SAFE
; RUN: ld.lld -r %t.o -o %t.partial
; RUN: ld.lld --avm-print-stack-usage %t.partial -o %t.relinked 2>&1 | FileCheck %s --check-prefix=SAFE
; RUN: not ld.lld -e overflow %t.o -o %t.over 2>&1 | FileCheck %s --check-prefix=OVER
; RUN: ld.lld -e unknown --avm-print-stack-gaps %t.o -o %t.unknown 2>&1 | FileCheck %s --check-prefix=UNKNOWN
; RUN: ld.lld -e recursive --avm-print-stack-gaps %t.o -o %t.recursive 2>&1 | FileCheck %s --check-prefix=RECURSIVE

; SAFE: complete bound: yes
; SAFE: large: frame
; SAFE-NOT: unresolved indirect
; OVER: error: AVM maximum provable stack usage is {{[0-9]+}} bytes; limit is 256 bytes
; OVER: huge: frame
; UNKNOWN: unresolved indirect call
; RECURSIVE: recursive call continuation

define void @_start(i1 %which) addrspace(1) {
  %f = select i1 %which, ptr addrspace(1) @small, ptr addrspace(1) @large
  call addrspace(1) void %f()
  ret void
}

define void @overflow(ptr addrspace(1) %f) addrspace(1) {
  %buf = alloca [40 x i8], align 1
  store volatile i8 1, ptr %buf
  call addrspace(1) void %f(), !callees !0
  store volatile i8 2, ptr %buf
  ret void
}

define void @unknown(ptr addrspace(1) %f) addrspace(1) {
  call addrspace(1) void %f()
  ret void
}

define void @recursive(ptr addrspace(1) %f) addrspace(1) {
  call addrspace(1) void %f(), !callees !1
  ret void
}

define void @small() addrspace(1) {
  %buf = alloca [8 x i8], align 1
  store volatile i8 1, ptr %buf
  ret void
}

define void @large() addrspace(1) {
  %buf = alloca [100 x i8], align 1
  store volatile i8 1, ptr %buf
  ret void
}

define void @huge() addrspace(1) {
  %buf = alloca [240 x i8], align 1
  store volatile i8 1, ptr %buf
  ret void
}

!0 = !{ptr addrspace(1) @small, ptr addrspace(1) @huge}
!1 = !{ptr addrspace(1) @small, ptr addrspace(1) @recursive}
