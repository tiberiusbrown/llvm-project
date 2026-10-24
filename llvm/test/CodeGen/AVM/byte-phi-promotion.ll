; RUN: llc -mtriple=avm -O2 -stop-after=avm-byte-phis %s -o - | FileCheck %s

; The whole PHI graph can share the native zero-extended representation.
; Signed users, memory and ABI operands still observe the original byte type.
declare void @take_byte(i8 signext)

define i8 @load_fed_switch(ptr addrspace(1) %p) {
; CHECK-LABEL: define i8 @load_fed_switch
; CHECK: %byte.word = phi i16
; CHECK: switch i16 %byte.word
; CHECK: i16 0, label %exit
; CHECK: i16 128, label %exit
; CHECK: load volatile i8, ptr addrspace(1)
entry:
  br label %loop
loop:
  %byte = phi i8 [255, %entry], [%loaded, %next]
  switch i8 %byte, label %next [i8 0, label %exit
                               i8 -128, label %exit]
next:
  %loaded = load volatile i8, ptr addrspace(1) %p
  br label %loop
exit:
  ret i8 %byte
}

define i32 @load_fed_cycle(ptr %p, i16 %count) {
; CHECK-LABEL: define i32 @load_fed_cycle
; CHECK: %a.word = phi i16
; CHECK: %b.word = phi i16
; CHECK: icmp slt i8
; CHECK: call {{.*}}void @take_byte(i8 signext
; CHECK: store volatile i8
; CHECK: %loaded = load volatile i8
; CHECK: zext i8 %loaded to i16
; CHECK: icmp ult i16
; CHECK: zext i16 %a.word to i32
entry:
  br label %loop
loop:
  %a = phi i8 [31, %entry], [%b, %next]
  %b = phi i8 [128, %entry], [%loaded, %next]
  %left = phi i16 [%count, %entry], [%remaining, %next]
  %negative = icmp slt i8 %a, 0
  br i1 %negative, label %signed, label %next
signed:
  call void @take_byte(i8 signext %a)
  br label %next
next:
  store volatile i8 %a, ptr %p
  %loaded = load volatile i8, ptr %p
  %remaining = sub i16 %left, 1
  %small = icmp ult i8 %a, 32
  %done = icmp eq i16 %remaining, 0
  %stop = or i1 %small, %done
  br i1 %stop, label %exit, label %loop
exit:
  %result = zext i8 %a to i32
  ret i32 %result
}

; An arithmetic input invalidates dependent PHIs as well. Widening these
; would need a mask to retain wrapping semantics, so it is not a free change.
define i8 @wrapping_cycle(ptr %p) {
; CHECK-LABEL: define i8 @wrapping_cycle
; CHECK-NOT: .word = phi
; CHECK: %a = phi i8
; CHECK: %b = phi i8
; CHECK: add i8
; CHECK: icmp
entry:
  br label %loop
loop:
  %a = phi i8 [250, %entry], [%b, %loop]
  %b = phi i8 [251, %entry], [%incremented, %loop]
  store volatile i8 %a, ptr %p
  %step = load volatile i8, ptr %p
  %incremented = add i8 %b, %step
  %again = icmp ne i8 %a, 5
  br i1 %again, label %loop, label %exit
exit:
  ret i8 %a
}
