; RUN: llc -mtriple=avm -O0 -verify-machineinstrs < %s | FileCheck %s
; RUN: llc -mtriple=avm -O2 -verify-machineinstrs < %s | FileCheck %s

; Integer offsets from converted stack addresses may wrap below zero. They
; must not be rejected as negative frame displacements during frame lowering.
define i16 @below_stack_address() nounwind {
; CHECK-LABEL: below_stack_address:
; CHECK-DAG: leasp
; CHECK-DAG: ldi16 {{r[0-7]}}, {{(-2561|62975)}}
; CHECK: add {{r[0-7]}},
; CHECK: ret
  %local = alloca [6 x i8], align 1
  %address = ptrtoint ptr %local to i16
  %result = add i16 %address, -2561
  ret i16 %result
}

define i16 @previous_byte() nounwind {
; CHECK-LABEL: previous_byte:
; CHECK: leasp
; CHECK: dec16
; CHECK: ret
  %local = alloca i8, align 1
  %address = ptrtoint ptr %local to i16
  %result = add i16 %address, -1
  ret i16 %result
}

define i16 @large_wrapping_offset() nounwind {
; CHECK-LABEL: large_wrapping_offset:
; CHECK-DAG: leasp
; CHECK-DAG: ldi16 {{r[0-7]}}, {{(-32768|32768)}}
; CHECK: {{(add|xor)}} {{r[0-7]}},
; CHECK: ret
  %local = alloca i8, align 1
  %address = ptrtoint ptr %local to i16
  %result = add i16 %address, -32768
  ret i16 %result
}

; Exercise the stable r3 base as well as SP. The comparison is the optimized
; form of an inclusive range check, as in the exception cursor encoder.
define i1 @address_range_with_fp() #0 nounwind {
; CHECK-LABEL: address_range_with_fp:
; CHECK: getsp r3
; CHECK: ldi16 {{r[0-7]}}, {{(-2561|62975)}}
; CHECK: add {{r[0-7]}},
; CHECK: cmp
; CHECK: ret
  %local = alloca [6 x i8], align 1
  %address = ptrtoint ptr %local to i16
  %shifted = add i16 %address, -2561
  %outside = icmp ult i16 %shifted, -254
  ret i1 %outside
}

attributes #0 = { "frame-pointer"="all" }
