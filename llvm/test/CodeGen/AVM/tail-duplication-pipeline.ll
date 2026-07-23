; RUN: llc -mtriple=avm -O2 -debug-pass=Structure -o %t %s 2>&1 \
; RUN:   | FileCheck %s --check-prefix=O2
; RUN: llc -mtriple=avm -O0 -debug-pass=Structure -o %t %s 2>&1 \
; RUN:   | FileCheck %s --check-prefix=O0 \
; RUN:       --implicit-check-not="AVM late tail duplication"

; O2:      AVM late tail duplication
; O2-NEXT: AVM measured branch polarity
; O2-NEXT: AVM post-RA pseudo instruction expansion
; O2-NEXT: Remove dead machine instructions
; O2-NEXT: AVM final control-flow cleanup

; O0: AVM measured branch polarity

define void @pipeline_probe() {
  ret void
}
