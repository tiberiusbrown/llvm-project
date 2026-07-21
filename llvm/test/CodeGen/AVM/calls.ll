; RUN: llc -mtriple=avm-unknown-arduboyfx -mcpu=avm1 \
; RUN:   -mtune=avm-interpreter-32u4-v1 -O0 -verify-machineinstrs < %s \
; RUN:   | FileCheck %s
; RUN: llc -mtriple=avm-unknown-arduboyfx -mcpu=avm1 \
; RUN:   -mtune=avm-interpreter-32u4-v1 -O0 -verify-machineinstrs \
; RUN:   -filetype=obj < %s -o %t.o
; RUN: llvm-readobj -r %t.o | FileCheck %s --check-prefix=RELOC
; RUN: llvm-objdump -d %t.o | FileCheck %s --check-prefix=OBJ

; RELOC:      R_AVM_RELAX
; RELOC-NEXT: R_AVM_FAR24 f0

; OBJ-LABEL: <call0>:
; OBJ:       jmpf
; OBJ-LABEL: <indirect>:
; OBJ:       callp

declare i16 @f0()
declare i16 @f1(i16)
declare i16 @f2(i16, i16)
declare i16 @f3(i16, i16, i16)
declare i16 @f4(i16, i16, i16, i16)
declare i16 @f5(i16, i16, i16, i16, i16)
declare i16 @f6(i16, i16, i16, i16, i16, i16)

define i16 @call0() {
; CHECK-LABEL: call0:
; CHECK:       jmp f0
  %r = call i16 @f0()
  ret i16 %r
}

define i16 @call1(i16 %a) {
; CHECK-LABEL: call1:
; CHECK:       jmp f1
  %r = call i16 @f1(i16 %a)
  ret i16 %r
}

define i16 @call2(i16 %a, i16 %b) {
; CHECK-LABEL: call2:
; CHECK:       jmp f2
  %r = call i16 @f2(i16 %a, i16 %b)
  ret i16 %r
}

define i16 @call3(i16 %a, i16 %b, i16 %c) {
; CHECK-LABEL: call3:
; CHECK:       jmp f3
  %r = call i16 @f3(i16 %a, i16 %b, i16 %c)
  ret i16 %r
}

define i16 @call4(i16 %a, i16 %b, i16 %c, i16 %d) {
; CHECK-LABEL: call4:
; CHECK:       jmp f4
  %r = call i16 @f4(i16 %a, i16 %b, i16 %c, i16 %d)
  ret i16 %r
}

define i16 @call5(i16 %a, i16 %b, i16 %c, i16 %d, i16 %e) {
; CHECK-LABEL: call5:
; CHECK:       push16 r0
; CHECK:       ldsp16 r0, [sp+5]
; CHECK-NEXT:  adjsp -2
; CHECK-NEXT:  stsp16 [sp+0], r0
; CHECK-NEXT:  call f5
; CHECK-NEXT:  adjsp 2
; CHECK-NEXT:  pop16 r0
; CHECK-NEXT:  ret
  %r = call i16 @f5(i16 %a, i16 %b, i16 %c, i16 %d, i16 %e)
  ret i16 %r
}

define i16 @call6(i16 %a, i16 %b, i16 %c, i16 %d, i16 %e, i16 %f) {
; CHECK-LABEL: call6:
; CHECK:       push16 r1
; CHECK-NEXT:  push16 r0
; CHECK-NEXT:  ldsp16 r1, [sp+9]
; CHECK-NEXT:  ldsp16 r0, [sp+7]
; CHECK-NEXT:  adjsp -4
; CHECK-NEXT:  stsp16 [sp+2], r1
; CHECK-NEXT:  stsp16 [sp+0], r0
; CHECK-NEXT:  call f6
; CHECK-NEXT:  adjsp 4
; CHECK-NEXT:  pop16 r0
; CHECK-NEXT:  pop16 r1
; CHECK-NEXT:  ret
  %r = call i16 @f6(i16 %a, i16 %b, i16 %c, i16 %d, i16 %e, i16 %f)
  ret i16 %r
}

declare i16 @mixed(i16, i32, i16)

define i16 @pair_alignment(i16 %a, i32 %b, i16 %c) {
; CHECK-LABEL: pair_alignment:
; CHECK:       ldsp16 r5, [sp+3]
; CHECK-NEXT:  adjsp -2
; CHECK-NEXT:  stsp16 [sp+0], r5
; CHECK-NEXT:  call mixed
; CHECK-NEXT:  adjsp 2
; CHECK-NEXT:  ret
  %r = call i16 @mixed(i16 %a, i32 %b, i16 %c)
  ret i16 %r
}

declare i16 @narrow(i16, i16, i16, i16, i8 signext, i8 zeroext)

define i16 @packed_narrow(i16 %a, i16 %b, i16 %c, i16 %d,
                          i8 signext %e, i8 zeroext %f) {
; CHECK-LABEL: packed_narrow:
; CHECK:       ldsp8u r1, [sp+8]
; CHECK-NEXT:  ldsp8s r0, [sp+7]
; CHECK-NEXT:  adjsp -2
; CHECK-NEXT:  stsp8 [sp+1], r1
; CHECK-NEXT:  stsp8 [sp+0], r0
; CHECK-NEXT:  call narrow
; CHECK-NEXT:  adjsp 2
  %r = call i16 @narrow(i16 %a, i16 %b, i16 %c, i16 %d,
                        i8 signext %e, i8 zeroext %f)
  ret i16 %r
}

define i16 @indirect(ptr addrspace(1) %fn, i16 %x) {
; CHECK-LABEL: indirect:
; CHECK:       callp q3
; CHECK:       ret
  %r = call addrspace(1) i16 %fn(i16 %x)
  ret i16 %r
}

declare void @take3(ptr addrspace(1), ptr addrspace(1), ptr addrspace(1))

define void @stacked_program_pointer(ptr addrspace(1) %a,
                                     ptr addrspace(1) %b,
                                     ptr addrspace(1) %c) {
; CHECK-LABEL: stacked_program_pointer:
; CHECK:       ldsp16 r0, [sp+7]
; CHECK-NEXT:  ldsp8u r1, [sp+9]
; CHECK-NEXT:  adjsp -3
; CHECK-NEXT:  stsp16 [sp+0], r0
; CHECK-NEXT:  stsp8 [sp+2], r1
; CHECK-NEXT:  zext8 r5
; CHECK-NEXT:  zext8 r7
; CHECK-NEXT:  call take3
; CHECK-NEXT:  adjsp 3
  call void @take3(ptr addrspace(1) %a, ptr addrspace(1) %b,
                   ptr addrspace(1) %c)
  ret void
}

declare void @clobber()
declare i16 @consume6(i16, i16, i16, i16, i16, i16)
declare i32 @consume3x32(i32, i32, i32)

define i16 @spill16(i16 %a, i16 %b, i16 %c, i16 %d, i16 %e, i16 %f) {
; CHECK-LABEL: spill16:
; CHECK:       push16 r1
; CHECK-NEXT:  push16 r0
; CHECK-NEXT:  adjsp -8
; CHECK:       stsp16 [sp+0], r4
; CHECK:       call clobber
; CHECK:       ldsp16 r4, [sp+0]
; CHECK:       call consume6
; CHECK:       adjsp 8
; CHECK-NEXT:  pop16 r0
; CHECK-NEXT:  pop16 r1
  call void @clobber()
  %r = call i16 @consume6(i16 %a, i16 %b, i16 %c, i16 %d, i16 %e, i16 %f)
  ret i16 %r
}

define i32 @spill32(i32 %a, i32 %b, i32 %c) {
; CHECK-LABEL: spill32:
; CHECK:       adjsp -8
; CHECK:       stsp16 [sp+0], r4
; CHECK-NEXT:  stsp16 [sp+2], r5
; CHECK:       call clobber
; CHECK:       ldsp16 r4, [sp+0]
; CHECK-NEXT:  ldsp16 r5, [sp+2]
; CHECK:       call consume3x32
  call void @clobber()
  %r = call i32 @consume3x32(i32 %a, i32 %b, i32 %c)
  ret i32 %r
}

declare void @close64(i16, i64, i16)

define void @four_unit_closes_registers(i16 %a, i64 %b, i16 %c) {
; CHECK-LABEL: four_unit_closes_registers:
; CHECK:       adjsp -10
; CHECK-DAG:   stsp16 [sp+0]
; CHECK-DAG:   stsp16 [sp+2]
; CHECK-DAG:   stsp16 [sp+4]
; CHECK-DAG:   stsp16 [sp+6]
; CHECK-DAG:   stsp16 [sp+8]
; CHECK:       call close64
; CHECK-NEXT:  adjsp 10
  call void @close64(i16 %a, i64 %b, i16 %c)
  ret void
}
