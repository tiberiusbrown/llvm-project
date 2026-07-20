; RUN: llc -mtriple=avm-unknown-arduboyfx -mcpu=avm1 \
; RUN:   -mtune=avm-interpreter-32u4-v1 -O0 -verify-machineinstrs < %s \
; RUN:   | FileCheck %s

%s5 = type { [5 x i8] }
%s6 = type { [6 x i8] }
%s7 = type { [7 x i8] }
%s8 = type { [8 x i8] }

define void @aggregate0() {
; CHECK-LABEL: aggregate0:
; CHECK:       ret
  ret void
}

define i8 @aggregate1(i8 %x) {
; CHECK-LABEL: aggregate1:
; CHECK:       ret
  ret i8 %x
}

define i16 @aggregate2(i16 %x) {
; CHECK-LABEL: aggregate2:
; CHECK:       ret
  ret i16 %x
}

define i24 @aggregate3(i24 %x) {
; CHECK-LABEL: aggregate3:
; CHECK:       ret
  ret i24 %x
}

define i32 @aggregate4(i32 %x) {
; CHECK-LABEL: aggregate4:
; CHECK:       ret
  ret i32 %x
}

define i24 @stack_aggregate3(i32 %a, i32 %b, i24 %x) {
; CHECK-LABEL: stack_aggregate3:
; CHECK:       ldsp16 {{.*}}, [sp+3]
; CHECK:       ldsp8u {{.*}}, [sp+5]
; CHECK:       ret
  ret i24 %x
}

declare void @consume_stack_aggregate3(i32, i32, i24)

define void @call_stack_aggregate3(i24 %x) {
; CHECK-LABEL: call_stack_aggregate3:
; CHECK:       adjsp -3
; CHECK:       stsp16 [sp+0]
; CHECK:       stsp8 [sp+2]
; CHECK:       call consume_stack_aggregate3
; CHECK-NEXT:  adjsp 3
; CHECK:       ret
  call void @consume_stack_aggregate3(i32 1, i32 2, i24 %x)
  ret void
}

define void @aggregate5(ptr noalias sret(%s5) align 1 %out,
                        ptr noundef byval(%s5) align 1 %in) {
; CHECK-LABEL: aggregate5:
; CHECK:       ret
  call void @llvm.memcpy.p0.p0.i16(ptr align 1 %out, ptr align 1 %in,
                                   i16 5, i1 false)
  ret void
}

define void @aggregate6(ptr noalias sret(%s6) align 1 %out,
                        ptr noundef byval(%s6) align 1 %in) {
; CHECK-LABEL: aggregate6:
; CHECK:       ret
  call void @llvm.memcpy.p0.p0.i16(ptr align 1 %out, ptr align 1 %in,
                                   i16 6, i1 false)
  ret void
}

define void @aggregate7(ptr noalias sret(%s7) align 1 %out,
                        ptr noundef byval(%s7) align 1 %in) {
; CHECK-LABEL: aggregate7:
; CHECK:       ret
  call void @llvm.memcpy.p0.p0.i16(ptr align 1 %out, ptr align 1 %in,
                                   i16 7, i1 false)
  ret void
}

define void @aggregate8(ptr noalias sret(%s8) align 1 %out,
                        ptr noundef byval(%s8) align 1 %in) {
; CHECK-LABEL: aggregate8:
; CHECK:       ret
  call void @llvm.memcpy.p0.p0.i16(ptr align 1 %out, ptr align 1 %in,
                                   i16 8, i1 false)
  ret void
}

declare void @consume_aggregate5(ptr noundef byval(%s5) align 1)

define void @call_aggregate5(ptr %source) {
; CHECK-LABEL: call_aggregate5:
; CHECK:       call consume_aggregate5
; CHECK:       ret
  call void @consume_aggregate5(ptr noundef byval(%s5) align 1 %source)
  ret void
}

declare void @consume_narrow(i8 signext, i8 zeroext, i1 zeroext)

define void @call_narrow(i16 %value) {
; CHECK-LABEL: call_narrow:
; CHECK:       sext8
; CHECK:       zext8
; CHECK:       call consume_narrow
; CHECK:       ret
  %signed = trunc i16 %value to i8
  %unsigned = trunc i16 %value to i8
  %boolean = icmp ne i16 %value, 0
  call void @consume_narrow(i8 signext %signed, i8 zeroext %unsigned,
                            i1 zeroext %boolean)
  ret void
}

define i64 @identity64(i64 %x) {
; CHECK-LABEL: identity64:
; CHECK:       ret
  ret i64 %x
}

declare ptr addrspace(1) @program_identity(ptr addrspace(1))

define ptr addrspace(1) @computed_program_pointer(ptr addrspace(1) %base,
                                                  i16 %offset) {
; CHECK-LABEL: computed_program_pointer:
; CHECK:       add32
; CHECK-NEXT:  zext8
; CHECK:       zext8
; CHECK:       call program_identity
; CHECK:       zext8
; CHECK:       ret
  %computed = getelementptr i8, ptr addrspace(1) %base, i16 %offset
  %returned = call ptr addrspace(1) @program_identity(
      ptr addrspace(1) %computed)
  ret ptr addrspace(1) %returned
}

define i1 @computed_program_equal(ptr addrspace(1) %base, i16 %offset,
                                  ptr addrspace(1) %other) {
; CHECK-LABEL: computed_program_equal:
; CHECK:       add32
; CHECK-NEXT:  zext8
; CHECK:       cmp32
; CHECK:       ret
  %computed = getelementptr i8, ptr addrspace(1) %base, i16 %offset
  %equal = icmp eq ptr addrspace(1) %computed, %other
  ret i1 %equal
}

declare void @llvm.memcpy.p0.p0.i16(ptr noalias writeonly captures(none),
                                    ptr noalias readonly captures(none), i16,
                                    i1 immarg)
