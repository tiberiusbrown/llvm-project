; RUN: llc -mtriple=avm-unknown-arduboyfx -mcpu=avm1 -mtune=avm-interpreter-32u4-v1 -O0 -verify-machineinstrs %s -o - | FileCheck %s
; RUN: llc -mtriple=avm-unknown-arduboyfx -mcpu=avm1 -mtune=avm-interpreter-32u4-v1 -O2 -verify-machineinstrs %s -o - | FileCheck %s
; RUN: llc -mtriple=avm -O0 -verify-machineinstrs -filetype=obj %s -o %t.o
; RUN: llvm-objdump -dr --no-print-imm-hex %t.o | FileCheck %s --check-prefix=OBJ
; RUN: llc -mtriple=avm -O2 -verify-machineinstrs -filetype=obj %s -o %t.opt.o
; RUN: llvm-objdump -dr --no-print-imm-hex %t.opt.o | FileCheck %s --check-prefix=OBJ
; RUN: llc -mtriple=avm -O0 -verify-machineinstrs -stop-after=finalize-isel %s -o - | FileCheck %s --check-prefix=ISEL

declare i16 @f1(i16)
declare i16 @f4(i16, i16, i16, i16)
declare i16 @f5(i16, i16, i16, i16, i16)
declare void @byval(ptr byval([4 x i8]))
declare void @vararg(...)
declare void @sret(ptr sret(i16))
declare i8 @small()

define i16 @framed_direct(i16 %x) {
; CHECK-LABEL: framed_direct:
; CHECK: adjsp -8
; CHECK-NOT: call f1
; CHECK: adjsp 8
; CHECK-NEXT: jmp f1
; CHECK-NOT: ret
; CHECK: .Lfunc_end
; OBJ-LABEL: <framed_direct>:
; OBJ: adjsp 8
; OBJ-NEXT: {{.*}}jmpf
; OBJ: R_AVM_RELAX
; OBJ: R_AVM_FAR24 f1
; ISEL-LABEL: name: framed_direct
; ISEL-NOT: ADJCALLSTACK
; ISEL: TAILCALL_DIRECT_PSEUDO @f1, 0, $r4, csr_avm_call
; ISEL-NOT: RET_PSEUDO
  %buf = alloca [8 x i8], align 1
  store volatile i8 1, ptr %buf
  %r = tail call i16 @f1(i16 %x)
  ret i16 %r
}

define i16 @saved_direct(i16 %x) {
; ISEL-LABEL: name: saved_direct
; CHECK-LABEL: saved_direct:
; CHECK: push16 r1
; CHECK-NEXT: push16 r0
; CHECK: pop16 r0
; CHECK-NEXT: pop16 r1
; CHECK-NEXT: jmp f1
; CHECK-NOT: ret
; CHECK: .Lfunc_end
  call void asm sideeffect "", "~{r0},~{r1}"()
  %r = tail call i16 @f1(i16 %x)
  ret i16 %r
}

define i16 @full_direct(i16 %a, i16 %b, i16 %c, i16 %d) {
; CHECK-LABEL: full_direct:
; CHECK: adjsp 1
; CHECK-NEXT: pop16 r0
; CHECK-NEXT: jmp f4
; CHECK-NOT: ret
; CHECK: .Lfunc_end
  %buf = alloca i8, align 1
  store volatile i8 1, ptr %buf
  %r = tail call i16 @f4(i16 %a, i16 %b, i16 %c, i16 %d)
  ret i16 %r
}

define i16 @five_arguments(i16 %x) {
; CHECK-LABEL: five_arguments:
; CHECK: adjsp -2
; CHECK: call f5
; CHECK: adjsp 2
; CHECK: ret
  %r = tail call i16 @f5(i16 %x, i16 2, i16 3, i16 4, i16 5)
  ret i16 %r
}

define i16 @framed_indirect(ptr addrspace(1) %fn, i16 %x) {
; CHECK-LABEL: framed_indirect:
; CHECK: adjsp -{{8|10}}
; CHECK-NOT: callp
; CHECK: adjsp {{8|10}}
; CHECK-NOT: callp
; CHECK: jmpp q3
; CHECK-NOT: ret
; CHECK: .Lfunc_end
; OBJ-LABEL: <framed_indirect>:
; OBJ: adjsp {{8|10}}
; OBJ-NOT: callp
; OBJ: jmpp q3
; ISEL-LABEL: name: framed_indirect
; ISEL: TAILCALL_INDIRECT_PSEUDO $r6r7, 0, $r4, csr_avm_call
  %buf = alloca [8 x i8], align 1
  store volatile i8 1, ptr %buf
  %r = tail call addrspace(1) i16 %fn(i16 %x)
  ret i16 %r
}

; Force the incoming target into a lower pair before copying it into q3.
define i16 @saved_indirect(ptr addrspace(1) %fn, i32 %x) {
; CHECK-LABEL: saved_indirect:
; CHECK: push16
; CHECK: pop16
; CHECK-NEXT: pop16
; CHECK-NEXT: jmpp q3
; CHECK-NOT: ret
; CHECK: .Lfunc_end
  %target = call ptr addrspace(1) asm sideeffect "", "={q0},0"(ptr addrspace(1) %fn)
  %r = tail call addrspace(1) i16 %target(i32 %x)
  ret i16 %r
}

; R4 and R6:R7 are occupied: the alignment hole at R5 is not a free pair.
define i16 @indirect_no_pair(ptr addrspace(1) %fn, i16 %x, i32 %y) {
; CHECK-LABEL: indirect_no_pair:
; CHECK: callp
; CHECK: pop16
; CHECK: ret
; ISEL-LABEL: name: indirect_no_pair
; ISEL: CALL_INDIRECT_PSEUDO
  %r = tail call addrspace(1) i16 %fn(i16 %x, i32 %y)
  ret i16 %r
}

define i16 @indirect_full(ptr addrspace(1) %fn) {
; CHECK-LABEL: indirect_full:
; CHECK: callp
; CHECK: pop16
; CHECK: ret
; ISEL-LABEL: name: indirect_full
; ISEL: CALL_INDIRECT_PSEUDO
  %r = tail call addrspace(1) i16 %fn(i16 1, i16 2, i16 3, i16 4)
  ret i16 %r
}

define void @reject_byval(ptr %src) {
; CHECK-LABEL: reject_byval:
; CHECK: call byval
; CHECK: ret
; ISEL-LABEL: name: reject_byval
; ISEL: CALL_DIRECT_PSEUDO @byval
  tail call void @byval(ptr byval([4 x i8]) %src)
  ret void
}

define void @reject_vararg_callee() {
; CHECK-LABEL: reject_vararg_callee:
; CHECK: call vararg
; CHECK: ret
; ISEL-LABEL: name: reject_vararg_callee
; ISEL: CALL_DIRECT_PSEUDO @vararg
  %buf = alloca i8, align 1
  store volatile i8 1, ptr %buf
  tail call void (...) @vararg()
  ret void
}

define void @reject_vararg_caller(...) {
; CHECK-LABEL: reject_vararg_caller:
; CHECK: call vararg
; CHECK: ret
  %buf = alloca i8, align 1
  store volatile i8 1, ptr %buf
  tail call void (...) @vararg()
  ret void
}

define void @reject_sret_caller(ptr sret(i16) %p) {
; CHECK-LABEL: reject_sret_caller:
; CHECK: call sret
; CHECK: ret
  tail call void @sret(ptr sret(i16) %p)
  ret void
}

define void @reject_sret_callee(ptr %p) {
; CHECK-LABEL: reject_sret_callee:
; CHECK: call sret
; CHECK: ret
  %buf = alloca i8, align 1
  store volatile i8 1, ptr %buf
  tail call void @sret(ptr sret(i16) %p)
  ret void
}

define i16 @reject_return_conversion() {
; CHECK-LABEL: reject_return_conversion:
; CHECK: call small
; CHECK: ret
  %buf = alloca i8, align 1
  store volatile i8 1, ptr %buf
  %r = tail call i8 @small()
  %wide = zext i8 %r to i16
  ret i16 %wide
}

define i16 @musttail_supported(i16 %x) {
; CHECK-LABEL: musttail_supported:
; CHECK: adjsp 1
; CHECK-NEXT: jmp f1
; CHECK-NOT: ret
; CHECK: .Lfunc_end
  %buf = alloca i8, align 1
  store volatile i8 1, ptr %buf
  %r = musttail call i16 @f1(i16 %x)
  ret i16 %r
}

define i16 @dynamic_tail(i16 %x) {
; CHECK-LABEL: dynamic_tail:
; CHECK: setsp r3
; CHECK: pop16 r3
; CHECK-NEXT: jmp f1
; CHECK-NOT: ret
; CHECK: .Lfunc_end
  %buf = alloca i8, i16 %x, align 1
  store volatile i8 1, ptr %buf
  %r = tail call i16 @f1(i16 %x)
  ret i16 %r
}

; Zero outgoing stack size alone is insufficient: no tail request.
define i16 @unmarked_framed(i16 %x) {
; CHECK-LABEL: unmarked_framed:
; CHECK: call f1
; CHECK: adjsp 1
; CHECK: ret
  %buf = alloca i8, align 1
  store volatile i8 1, ptr %buf
  %r = call i16 @f1(i16 %x)
  ret i16 %r
}

; A tail hint with work after the call is not in tail position.
define i16 @not_tail_position(i16 %x) {
; CHECK-LABEL: not_tail_position:
; CHECK: call f1
; CHECK: ret
  %r = tail call i16 @f1(i16 %x)
  %s = add i16 %r, 1
  ret i16 %s
}
