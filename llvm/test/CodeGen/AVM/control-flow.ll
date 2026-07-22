; RUN: llc -mtriple=avm-unknown-arduboyfx -mcpu=avm1 \
; RUN:   -mtune=avm-interpreter-32u4-v1 -O0 -verify-machineinstrs < %s \
; RUN:   | FileCheck %s --check-prefix=O0
; RUN: llc -mtriple=avm-unknown-arduboyfx -mcpu=avm1 \
; RUN:   -mtune=avm-interpreter-32u4-v1 -O2 -verify-machineinstrs < %s \
; RUN:   | FileCheck %s --check-prefixes=O2,SIZE

define i16 @branch_ult(i16 %a, i16 %b) {
; O0-LABEL: branch_ult:
; O0:       cmp
; O0-NEXT:  bruge
; O0-NOT:   cset
; O2-LABEL: branch_ult:
; O2:       cmp r4, r5
; O2-NEXT:  bruge
; O2-NOT:   cset
  %cmp = icmp ult i16 %a, %b
  br i1 %cmp, label %yes, label %no
yes:
  ret i16 1
no:
  ret i16 0
}

define i16 @loop(i16 %n) {
; O0-LABEL: loop:
; O0:       cmp
; O0-NEXT:  bruge
; O0:       jmp
; O2-LABEL: loop:
; O2:       cmp
; O2-NEXT:  bruge
  br label %header
header:
  %i = phi i16 [ 0, %0 ], [ %next, %body ]
  %more = icmp ult i16 %i, %n
  br i1 %more, label %body, label %exit
body:
  %next = add i16 %i, 1
  br label %header
exit:
  ret i16 %i
}

define zeroext i1 @zero_i8(i8 %value) {
; O2-LABEL: zero_i8:
; O2:       tst8 r4
; O2-NEXT:  cset.eq r4
  %cmp = icmp eq i8 %value, 0
  ret i1 %cmp
}

define i16 @zero_i16(i16 %value) {
; O2-LABEL: zero_i16:
; O2:       tst16 r4
; O2-NEXT:  cset.ne r4
  %cmp = icmp ne i16 %value, 0
  %result = zext i1 %cmp to i16
  ret i16 %result
}

define zeroext i1 @eq_i8(i8 %a, i8 %b) {
; O2-LABEL: eq_i8:
; O2:       cset.eq
  %cmp = icmp eq i8 %a, %b
  ret i1 %cmp
}

define zeroext i1 @ne_i8(i8 %a, i8 %b) {
; O2-LABEL: ne_i8:
; O2:       cset.ne
  %cmp = icmp ne i8 %a, %b
  ret i1 %cmp
}

define zeroext i1 @ult_i8(i8 %a, i8 %b) {
; O2-LABEL: ult_i8:
; O2:       cset.ult
  %cmp = icmp ult i8 %a, %b
  ret i1 %cmp
}

define zeroext i1 @uge_i8(i8 %a, i8 %b) {
; O2-LABEL: uge_i8:
; O2:       cset.uge
  %cmp = icmp uge i8 %a, %b
  ret i1 %cmp
}

define zeroext i1 @slt_i8(i8 %a, i8 %b) {
; O2-LABEL: slt_i8:
; O2:       cset.slt
  %cmp = icmp slt i8 %a, %b
  ret i1 %cmp
}

define zeroext i1 @sge_i8(i8 %a, i8 %b) {
; O2-LABEL: sge_i8:
; O2:       cset.sge
  %cmp = icmp sge i8 %a, %b
  ret i1 %cmp
}

define i16 @eq(i16 %a, i16 %b) {
; O2-LABEL: eq:
; O2:       cmp r4, r5
; O2-NEXT:  cset.eq r4
  %cmp = icmp eq i16 %a, %b
  %result = zext i1 %cmp to i16
  ret i16 %result
}

define i16 @ne(i16 %a, i16 %b) {
; O2-LABEL: ne:
; O2:       cset.ne
  %cmp = icmp ne i16 %a, %b
  %result = zext i1 %cmp to i16
  ret i16 %result
}

define i16 @ult(i16 %a, i16 %b) {
; O2-LABEL: ult:
; O2:       cset.ult
  %cmp = icmp ult i16 %a, %b
  %result = zext i1 %cmp to i16
  ret i16 %result
}

define i16 @uge(i16 %a, i16 %b) {
; O2-LABEL: uge:
; O2:       cset.uge
  %cmp = icmp uge i16 %a, %b
  %result = zext i1 %cmp to i16
  ret i16 %result
}

define i16 @slt(i16 %a, i16 %b) {
; O2-LABEL: slt:
; O2:       cset.slt
  %cmp = icmp slt i16 %a, %b
  %result = zext i1 %cmp to i16
  ret i16 %result
}

define i16 @sge(i16 %a, i16 %b) {
; O2-LABEL: sge:
; O2:       cset.sge
  %cmp = icmp sge i16 %a, %b
  %result = zext i1 %cmp to i16
  ret i16 %result
}

define i16 @ule(i16 %a, i16 %b) {
; O2-LABEL: ule:
; O2:       cmp r5, r4
; O2-NEXT:  cset.uge
  %cmp = icmp ule i16 %a, %b
  %result = zext i1 %cmp to i16
  ret i16 %result
}

define i16 @ugt(i16 %a, i16 %b) {
; O2-LABEL: ugt:
; O2:       cmp r5, r4
; O2-NEXT:  cset.ult
  %cmp = icmp ugt i16 %a, %b
  %result = zext i1 %cmp to i16
  ret i16 %result
}

define i16 @immediate(i16 %a) {
; O2-LABEL: immediate:
; O2:       cmpi.s8 r4, 42
; O2-NEXT:  cset.slt r4
  %cmp = icmp slt i16 %a, 42
  %result = zext i1 %cmp to i16
  ret i16 %result
}

define i16 @immediate_negative(i16 %a) {
; O2-LABEL: immediate_negative:
; O2:       cmpi.s8 r4, -5
; O2-NEXT:  cset.slt r4
  %cmp = icmp slt i16 %a, -5
  %result = zext i1 %cmp to i16
  ret i16 %result
}

define i16 @select_i16(i16 %a, i16 %b, i16 %x, i16 %y) {
; O2-LABEL: select_i16:
; O2:       cmp
; O2-NEXT:  cmov.slt
; O2-NOT:   br
  %cmp = icmp slt i16 %a, %b
  %result = select i1 %cmp, i16 %x, i16 %y
  ret i16 %result
}

define zeroext i8 @select_i8(i8 %a, i8 %b, i8 %x, i8 %y) {
; O2-LABEL: select_i8:
; O2:       cmp
; O2-NEXT:  cmov.ult
; O2-NOT:   br
  %cmp = icmp ult i8 %a, %b
  %result = select i1 %cmp, i8 %x, i8 %y
  ret i8 %result
}

define i32 @select_i32(i16 %a, i16 %b, i32 %x, i32 %y) {
; O2-LABEL: select_i32:
; O2:       cmp
; O2-COUNT-2: cmov.ult
; O2-NOT:   br
  %cmp = icmp ult i16 %a, %b
  %result = select i1 %cmp, i32 %x, i32 %y
  ret i32 %result
}

define i16 @size_select_i16(i16 %a, i16 %b, i16 %x, i16 %y) #0 {
; SIZE-LABEL: size_select_i16:
; SIZE:       cmp
; SIZE-NEXT:  cmov.eq
; SIZE-NOT:   br
  %cmp = icmp eq i16 %a, %b
  %result = select i1 %cmp, i16 %x, i16 %y
  ret i16 %result
}

define i32 @minsize_select_i32(i16 %a, i16 %b, i32 %x, i32 %y) #1 {
; SIZE-LABEL: minsize_select_i32:
; SIZE:       cmp
; SIZE-COUNT-2: cmov.eq
; SIZE-NOT:   br
  %cmp = icmp ne i16 %a, %b
  %result = select i1 %cmp, i32 %x, i32 %y
  ret i32 %result
}

define i16 @machine_if_convert(i16 %cond, i16 %x, i16 %y) {
; O2-LABEL: machine_if_convert:
; O2:       tst16
; O2-NEXT:  cmov.eq
; O2-NOT:   br
entry:
  %test = icmp ne i16 %cond, 0
  br i1 %test, label %true, label %false
true:
  br label %join
false:
  br label %join
join:
  %result = phi i16 [ %x, %true ], [ %y, %false ]
  ret i16 %result
}

define i16 @no_arithmetic_if_convert(i16 %cond, i16 %x, i16 %y) {
; O2-LABEL: no_arithmetic_if_convert:
; O2:       tst16
; O2-NEXT:  breq
; O2-NOT:   cmov
entry:
  %test = icmp ne i16 %cond, 0
  br i1 %test, label %true, label %false
true:
  %sum = add i16 %x, 3
  br label %join
false:
  br label %join
join:
  %result = phi i16 [ %sum, %true ], [ %y, %false ]
  ret i16 %result
}

declare void @callee()

define i16 @compare_across_call(i16 %a, i16 %b) {
; O2-LABEL: compare_across_call:
; O2:       call callee
; O2:       cmp
; O2-NEXT:  bruge
  %cmp = icmp ult i16 %a, %b
  call void @callee()
  br i1 %cmp, label %yes, label %no
yes:
  ret i16 7
no:
  ret i16 9
}

define i16 @hot_true(i16 %a, i16 %b) {
; O2-LABEL: hot_true:
; O2:       cmp
; O2-NEXT:  bruge
  %cmp = icmp ult i16 %a, %b
  br i1 %cmp, label %yes, label %no, !prof !0
yes:
  %inc = add i16 %a, 1
  ret i16 %inc
no:
  %dec = sub i16 %a, 1
  ret i16 %dec
}

define i16 @hot_false(i16 %a, i16 %b) {
; O2-LABEL: hot_false:
; O2:       cmp
; O2-NEXT:  brult
  %cmp = icmp ult i16 %a, %b
  br i1 %cmp, label %yes, label %no, !prof !1
yes:
  %inc = add i16 %a, 1
  ret i16 %inc
no:
  %dec = sub i16 %a, 1
  ret i16 %dec
}

define i16 @switch_tree(i16 %value) {
; O2-LABEL: switch_tree:
; O2-NOT:   .rodata
; O2-NOT:   jmpp
; O2:       cmp
; O2:       br
  switch i16 %value, label %default [
    i16 0, label %case0
    i16 1, label %case1
    i16 2, label %case2
    i16 3, label %case3
    i16 4, label %case4
  ]
case0:
  ret i16 10
case1:
  ret i16 11
case2:
  ret i16 12
case3:
  ret i16 13
case4:
  ret i16 14
default:
  ret i16 15
}

attributes #0 = { optsize }
attributes #1 = { minsize optsize }

!0 = !{!"branch_weights", i32 100, i32 1}
!1 = !{!"branch_weights", i32 1, i32 100}
