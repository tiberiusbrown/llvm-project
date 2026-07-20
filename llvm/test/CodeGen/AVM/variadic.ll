; RUN: llc -mtriple=avm-unknown-arduboyfx -mcpu=avm1 \
; RUN:   -mtune=avm-interpreter-32u4-v1 -O0 -verify-machineinstrs < %s \
; RUN:   | FileCheck %s

declare void @llvm.va_start.p0(ptr)
declare void @llvm.va_copy.p0(ptr, ptr)
declare void @llvm.va_end.p0(ptr)

define i16 @read_variadic(i16 %named, ...) {
; CHECK-LABEL: read_variadic:
; CHECK:       ldsp16 {{.*}}, [sp+7]
; CHECK:       leasp {{.*}}, 9
; CHECK:       addi.s8 {{.*}}, 2
; CHECK:       ld16 {{.*}}, [{{.*}}]
; CHECK:       addi.s8 {{.*}}, 4
; CHECK:       ld16 {{.*}}, [{{.*}}]
; CHECK:       ret
  %ap = alloca ptr, align 1
  %copy = alloca ptr, align 1
  call void @llvm.va_start.p0(ptr %ap)
  %argp0 = load ptr, ptr %ap, align 1
  %next0 = getelementptr i8, ptr %argp0, i16 2
  store ptr %next0, ptr %ap, align 1
  %a = load i16, ptr %argp0, align 1
  %argp1 = load ptr, ptr %ap, align 1
  %next1 = getelementptr i8, ptr %argp1, i16 4
  store ptr %next1, ptr %ap, align 1
  %b = load i32, ptr %argp1, align 1
  call void @llvm.va_copy.p0(ptr %copy, ptr %ap)
  call void @llvm.va_end.p0(ptr %copy)
  call void @llvm.va_end.p0(ptr %ap)
  %bt = trunc i32 %b to i16
  %sum = add i16 %named, %a
  %result = add i16 %sum, %bt
  ret i16 %result
}

declare i16 @callee(i16, ...)

define i16 @call_variadic(ptr addrspace(1) %program) {
; CHECK-LABEL: call_variadic:
; CHECK:       adjsp -11
; CHECK-DAG:   stsp16 [sp+0]
; CHECK-DAG:   stsp16 [sp+2]
; CHECK-DAG:   stsp16 [sp+4]
; CHECK-DAG:   stsp16 [sp+6]
; CHECK-DAG:   stsp16 [sp+8]
; CHECK-DAG:   stsp8 [sp+10]
; CHECK:       call callee
; CHECK-NEXT:  adjsp 11
  %result = call i16 (i16, ...) @callee(i16 1, i16 2, i32 3,
                                        ptr addrspace(1) %program)
  ret i16 %result
}
