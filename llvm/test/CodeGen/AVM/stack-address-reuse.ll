; RUN: llc -mtriple=avm -O2 -verify-machineinstrs %s -o - | FileCheck %s
; RUN: llc -mtriple=avm -O2 -stop-after=finalize-isel -verify-machineinstrs %s -o - | FileCheck %s --check-prefix=ISEL
; RUN: llc -mtriple=avm -O2 -stop-after=early-machinelicm -verify-machineinstrs %s -o - | FileCheck %s --check-prefix=LICM

declare void @fill(ptr)
declare void @take(ptr, i16, i16, i16, i16, i16)

; The same fixed object is indexed in multiple paths of a nested loop. Keep
; its base separate from the destructive index addition, and share it between
; the read and write paths without forcing other loop invariants to spill.
define i16 @nested_stack(i16 %limit) {
; CHECK-LABEL: nested_stack:
; CHECK: leasp [[BASE:r[0-7]]], 0
; CHECK-NOT: leasp
; CHECK: call fill
; CHECK-NOT: leasp
; CHECK: %inner
; CHECK-NOT: leasp
; CHECK: mov [[READ:r[0-7]]], [[BASE]]
; CHECK-NEXT: add [[READ]], {{r[0-7]}}
; CHECK-NEXT: ld8u
; CHECK-NOT: leasp
; CHECK: %write
; CHECK-NOT: leasp
; CHECK: mov [[WRITE:r[0-7]]], [[BASE]]
; CHECK-NEXT: add [[WRITE]], {{r[0-7]}}
; CHECK-NEXT: st8
; CHECK-NOT: leasp
; CHECK: ret
; ISEL-LABEL: name: nested_stack
; ISEL: FRAMEADDR_PSEUDO %stack.0.array, 0{{$}}
; ISEL: bb.2.inner:
; ISEL: FRAMEADDR_PSEUDO %stack.0.array, 0{{$}}
; ISEL: bb.3.write:
; ISEL: FRAMEADDR_PSEUDO %stack.0.array, 0{{$}}
entry:
  %array = alloca [8 x i8], align 1
  call void @fill(ptr %array)
  br label %outer
outer:
  %round = phi i16 [0, %entry], [%round.next, %done]
  br label %inner
inner:
  %index = phi i16 [7, %outer], [%previous, %write]
  %address = getelementptr [8 x i8], ptr %array, i16 0, i16 %index
  %value = load volatile i8, ptr %address, align 1
  %stop = icmp eq i8 %value, 0
  br i1 %stop, label %done, label %write
write:
  %previous = add i16 %index, -1
  %destination = getelementptr [8 x i8], ptr %array, i16 0, i16 %previous
  store volatile i8 %value, ptr %destination, align 1
  %first = icmp eq i16 %previous, 0
  br i1 %first, label %done, label %inner
done:
  %wide = zext i8 %value to i16
  %round.next = add i16 %round, 1
  %finished = icmp eq i16 %round.next, %limit
  br i1 %finished, label %exit, label %outer
exit:
  ret i16 %wide
}

; Outgoing arguments temporarily change SP. A fixed object's address still
; denotes that object and must include the call-frame adjustment at lowering.
define void @outgoing_arguments(i16 %index) {
; CHECK-LABEL: outgoing_arguments:
; CHECK: adjsp -4
; CHECK: leasp {{r[0-7]}}, 4
; CHECK: call take
; CHECK-NEXT: adjsp 4
; CHECK-NOT: leasp
; CHECK: st8
; CHECK: ret
; ISEL-LABEL: name: outgoing_arguments
; ISEL: ADJCALLSTACKDOWN 4,
; ISEL: FRAMEADDR_PSEUDO %stack.0.array, 0{{$}}
; ISEL: CALL_DIRECT_PSEUDO @take, 4,
entry:
  %array = alloca [8 x i8], align 1
  %address = getelementptr [8 x i8], ptr %array, i16 0, i16 %index
  call void @take(ptr %address, i16 1, i16 2, i16 3, i16 4, i16 5)
  store volatile i8 42, ptr %address, align 1
  ret void
}

; Variable allocation changes SP inside the loop. Frame-index lowering must
; use the stable frame pointer for the fixed object's shared address.
define i16 @dynamic_stack(i16 %size) {
; CHECK-LABEL: dynamic_stack:
; CHECK: getsp r3
; CHECK: mov [[FIXED:r[0-7]]], r3
; CHECK-NEXT: addi.s8 [[FIXED]], 2
; CHECK: %loop
; CHECK-NOT: leasp
; CHECK: getsp [[DYNAMIC:r[0-7]]]
; CHECK: sub [[DYNAMIC]],
; CHECK-NEXT: setsp [[DYNAMIC]]
; CHECK: call fill
; CHECK: ld8u {{r[0-7]}}, [{{r[0-7]}}+]
; CHECK: setsp r3
; CHECK: ret
; ISEL-LABEL: name: dynamic_stack
; ISEL: FRAMEADDR_PSEUDO %stack.0.array, 0{{$}}
entry:
  %array = alloca [8 x i8], align 1
  call void @fill(ptr %array)
  br label %loop
loop:
  %index = phi i16 [0, %entry], [%next, %loop]
  %sum = phi i16 [0, %entry], [%sum.next, %loop]
  %dynamic = alloca i8, i16 %size, align 1
  call void @fill(ptr %dynamic)
  %address = getelementptr [8 x i8], ptr %array, i16 0, i16 %index
  %value = load volatile i8, ptr %address, align 1
  %wide = zext i8 %value to i16
  %sum.next = add i16 %sum, %wide
  %next = add i16 %index, 1
  %done = icmp eq i16 %next, 8
  br i1 %done, label %exit, label %loop
exit:
  ret i16 %sum.next
}

; An address used only as a call argument need not be kept across iterations.
; Hoisting it can trade LEASP for a spill/reload and enlarge the stack frame.
define void @call_argument_only(i16 %limit) {
; CHECK-LABEL: call_argument_only:
; CHECK: call take
; CHECK: ret
; ISEL-LABEL: name: call_argument_only
; ISEL: FRAMEADDR_PSEUDO %stack.0.array, 0{{$}}
; LICM-LABEL: name: call_argument_only
; LICM-NOT: FRAMEADDR_PSEUDO
; LICM: bb.1.loop:
; LICM: FRAMEADDR_PSEUDO %stack.0.array, 0{{$}}
entry:
  %array = alloca [8 x i8], align 1
  br label %loop
loop:
  %index = phi i16 [0, %entry], [%next, %loop]
  call void @take(ptr %array, i16 1, i16 2, i16 3, i16 4, i16 5)
  %next = add i16 %index, 1
  %done = icmp eq i16 %next, %limit
  br i1 %done, label %exit, label %loop
exit:
  ret void
}

; An indexed base also needs to stay local when hoisting would keep it live
; across calls and reduce the already small preserved-register set.
define void @indexed_call_loop(i16 %limit) {
; CHECK-LABEL: indexed_call_loop:
; CHECK: call fill
; CHECK: ret
; ISEL-LABEL: name: indexed_call_loop
; ISEL: FRAMEADDR_PSEUDO %stack.0.array, 0{{$}}
; LICM-LABEL: name: indexed_call_loop
; LICM-NOT: FRAMEADDR_PSEUDO
; LICM: bb.1.loop:
; LICM: FRAMEADDR_PSEUDO %stack.0.array, 0{{$}}
entry:
  %array = alloca [8 x i8], align 1
  br label %loop
loop:
  %index = phi i16 [0, %entry], [%next, %loop]
  %small = and i16 %index, 7
  %address = getelementptr [8 x i8], ptr %array, i16 0, i16 %small
  call void @fill(ptr %address)
  %next = add i16 %index, 1
  %done = icmp eq i16 %next, %limit
  br i1 %done, label %exit, label %loop
exit:
  ret void
}
