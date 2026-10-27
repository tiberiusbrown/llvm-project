; RUN: llc -mtriple=avm -O0 -function-sections -filetype=obj %s -o %t.o
; RUN: ld.lld --gc-sections --avm-print-stack-usage %t.o -o %t.safe 2>&1 | FileCheck %s --check-prefix=SAFE
; RUN: ld.lld --gc-sections -e unknown --avm-print-stack-usage %t.o -o %t.unknown 2>&1 | FileCheck %s --check-prefix=UNKNOWN
; SAFE: complete bound: yes
; UNKNOWN: complete bound: no
; Registration coverage must use addresses, so an alias cannot disguise a
; registration call or turn an unknown callback into a complete bound.
@alias = alias i16 (ptr addrspace(1)), ptr addrspace(1) @atexit

define i16 @atexit(ptr addrspace(1) %fn) addrspace(1) noinline {
  ret i16 0
}
define void @_start(ptr addrspace(1) %dispatch) addrspace(1) {
  %r = call addrspace(1) i16 @alias(ptr addrspace(1) @callback)
  call addrspace(1) void %dispatch(), !avm.stack.dispatch !0
  ret void
}
define void @unknown(ptr addrspace(1) %fn) addrspace(1) {
  %r = call addrspace(1) i16 @alias(ptr addrspace(1) %fn)
  call addrspace(1) void %fn(), !avm.stack.dispatch !0
  ret void
}
define void @callback() addrspace(1) { ret void }
!0 = !{i32 64}

