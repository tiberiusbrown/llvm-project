; RUN: llc -mtriple=avm -O2 -verify-machineinstrs < %s \
; RUN:   | FileCheck %s

@input_a = external global [16 x float], align 1
@input_b = external global [16 x float], align 1

define float @reuse_half_constant(float %initial) #0 {
; CHECK-LABEL: reuse_half_constant:
; CHECK:       ldi16 [[HALF1:r[0-7]]], 16128
; CHECK-NOT:   stsp16 {{.*}}, [[HALF1]]
; CHECK-NOT:   ldsp16 [[HALF1]], {{.*}}
; CHECK:       ldi16 [[HALF2:r[0-7]]], 16128
entry:
  br label %loop

loop:
  %index = phi i16 [ 0, %entry ], [ %next, %loop ]
  %acc = phi float [ %initial, %entry ], [ %acc.next, %loop ]

  %a.ptr =
      getelementptr inbounds [16 x float], ptr @input_a, i16 0, i16 %index
  %b.ptr =
      getelementptr inbounds [16 x float], ptr @input_b, i16 0, i16 %index
  %a = load volatile float, ptr %a.ptr, align 1
  %b = load volatile float, ptr %b.ptr, align 1

  %product = fmul float %a, %b
  %biased = fadd float %product, 1.250000e+00
  %with.half = fadd float %biased, 5.000000e-01

  %below = fcmp olt float %with.half, -2.000000e+00
  %low.clamp =
      select i1 %below, float -2.000000e+00, float %with.half

  %above = fcmp ogt float %low.clamp, 3.000000e+00
  %clamped =
      select i1 %above, float 3.000000e+00, float %low.clamp

  %index.float = uitofp i16 %index to float
  %with.index = fadd float %clamped, %index.float
  %with.second.half = fadd float %with.index, 5.000000e-01
  %acc.next = fadd float %acc, %with.second.half

  %next = add nuw i16 %index, 1
  %done = icmp eq i16 %next, 16
  br i1 %done, label %exit, label %loop

exit:
  ret float %acc.next
}

attributes #0 = { optsize }
