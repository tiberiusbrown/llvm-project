; RUN: split-file %s %t
; RUN: not --crash llc -mtriple=avm-unknown-arduboyfx -mcpu=avm1 \
; RUN:   -mtune=avm-interpreter-32u4-v1 -O2 %t/store.ll -o /dev/null 2>&1 \
; RUN:   | FileCheck %s --check-prefix=STORE
; RUN: not --crash llc -mtriple=avm-unknown-arduboyfx -mcpu=avm1 \
; RUN:   -mtune=avm-interpreter-32u4-v1 -O2 %t/cast.ll -o /dev/null 2>&1 \
; RUN:   | FileCheck %s --check-prefix=CAST

; STORE: LLVM ERROR: AVM address space 1 is read-only
; CAST: LLVM ERROR: AVM does not support casts between address spaces 0 and 1

;--- store.ll
define void @store_to_program_memory(ptr addrspace(1) %address, i8 %value) {
  store i8 %value, ptr addrspace(1) %address, align 1
  ret void
}

;--- cast.ll
define ptr @cast_program_pointer(ptr addrspace(1) %address) {
  %result = addrspacecast ptr addrspace(1) %address to ptr
  ret ptr %result
}
