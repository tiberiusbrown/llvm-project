; RUN: llc -mtriple=avm < %s | FileCheck %s

declare i16 @llvm.avm.debug.printfv.p(ptr addrspace(1), ptr)

define i16 @debug_printfv_p(ptr addrspace(1) %format, ptr %ap) {
entry:
  %result = call i16 @llvm.avm.debug.printfv.p(ptr addrspace(1) %format,
                                               ptr %ap)
  ret i16 %result
}

; CHECK-LABEL: debug_printfv_p:
; CHECK: sys{{[ \t]+}}debug_printfv_p
; CHECK: ret
