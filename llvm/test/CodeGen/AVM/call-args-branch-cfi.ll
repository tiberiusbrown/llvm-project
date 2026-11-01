; RUN: llc -mtriple=avm-unknown-arduboyfx -O2 -stop-after=branch-folder %s -o - | FileCheck %s --check-prefix=MIR

; Each branch calls a function with stack arguments. Branch folding must not
; hoist the outgoing ADJSP above the branch or share the cleanup ADJSP between
; the branches, because the CFI row for each adjustment belongs to its block.

declare i16 @many(i16, i16, i16, i16, i16, i16, i16, i16, i16, i16)
declare i16 @many_alt(i16, i16, i16, i16, i16, i16, i16, i16, i16, i16)
declare i16 @leaf(i16)

; MIR-LABEL: name: join_args
; MIR:      bb.0
; MIR-NOT:  ADJSP
; MIR:      BR_CC_PSEUDO
; MIR:      bb.{{[0-9]+}}
; MIR:      nomerge ADJSP -12
; MIR-NEXT: nomerge CFI_INSTRUCTION def_cfa_offset 19
; MIR:      nomerge ADJSP 12
; MIR:      bb.{{[0-9]+}}
; MIR:      nomerge ADJSP -12
; MIR-NEXT: nomerge CFI_INSTRUCTION def_cfa_offset 19
; MIR:      nomerge ADJSP 12
define i16 @join_args(i16 %value) noinline {
entry:
  %bit = and i16 %value, 1
  %odd = icmp ne i16 %bit, 0
  br i1 %odd, label %then, label %else
then:
  %a = call i16 @many(i16 %value, i16 2, i16 3, i16 4, i16 5, i16 6, i16 7, i16 8, i16 9, i16 10)
  br label %join
else:
  %b = call i16 @many_alt(i16 10, i16 9, i16 8, i16 7, i16 6, i16 5, i16 4, i16 3, i16 2, i16 %value)
  br label %join
join:
  %r = phi i16 [ %a, %then ], [ %b, %else ]
  %l = call i16 @leaf(i16 %value)
  %s = add i16 %r, %l
  ret i16 %s
}

; The same argument setup on the return paths: each branch ends with its own
; cleanup and return, so no path may enter a block with its arguments still
; allocated.
; MIR-LABEL: name: return_args
; MIR:      bb.0
; MIR-NOT:  ADJSP
; MIR:      BR_CC_PSEUDO
; MIR:      nomerge ADJSP -12
; MIR:      nomerge ADJSP 12
; MIR:      bb.{{[0-9]+}}
; MIR:      nomerge ADJSP -12
; MIR:      nomerge ADJSP 12
define i16 @return_args(i16 %value) noinline {
entry:
  %bit = and i16 %value, 1
  %odd = icmp ne i16 %bit, 0
  br i1 %odd, label %then, label %else
then:
  %a = call i16 @many(i16 %value, i16 2, i16 3, i16 4, i16 5, i16 6, i16 7, i16 8, i16 9, i16 10)
  ret i16 %a
else:
  %b = call i16 @many_alt(i16 10, i16 9, i16 8, i16 7, i16 6, i16 5, i16 4, i16 3, i16 2, i16 %value)
  ret i16 %b
}
