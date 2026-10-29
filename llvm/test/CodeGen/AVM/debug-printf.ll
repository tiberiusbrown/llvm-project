; RUN: opt -passes="default<O2>" -S %s | FileCheck %s --check-prefix=OPT
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

declare i16 @llvm.avm.debug.printfv(ptr, ptr)

define i16 @debug_printfv(ptr %format, ptr %ap) {
entry:
  %result = call i16 @llvm.avm.debug.printfv(ptr %format,
                                               ptr %ap)
  ret i16 %result
}

; CHECK-LABEL: debug_printfv:
; CHECK: sys{{[ \t]+}}debug_printfv
; CHECK: ret

; OPT-LABEL: define i16 @percent_n_order(
; OPT: store i16 7, ptr %n
; OPT: call i16 @llvm.avm.debug.printfv(
; OPT: load i16, ptr %n
; OPT: ret i16
; CHECK-LABEL: percent_n_order:
; CHECK: sys{{[ \t]+}}debug_printfv
; CHECK: ret
define i16 @percent_n_order(ptr %format, ptr %ap, ptr %n) {
  store i16 7, ptr %n, align 1
  %count = call i16 @llvm.avm.debug.printfv(ptr %format, ptr %ap)
  %observed = load i16, ptr %n, align 1
  %result = add i16 %count, %observed
  ret i16 %result
}

; OPT-LABEL: define void @debug_observable(
; OPT: call i16 @llvm.avm.debug.printfv(
; OPT: call i16 @llvm.avm.debug.printfv(
; OPT: ret void
; CHECK-LABEL: debug_observable:
; CHECK: sys{{[ \t]+}}debug_printfv
; CHECK: sys{{[ \t]+}}debug_printfv
; CHECK: ret
define void @debug_observable(ptr %format, ptr %ap) {
  %first = call i16 @llvm.avm.debug.printfv(ptr %format, ptr %ap)
  %second = call i16 @llvm.avm.debug.printfv(ptr %format, ptr %ap)
  ret void
}
