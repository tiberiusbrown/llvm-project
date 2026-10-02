; RUN: not --crash llc -mtriple=avm -O0 -verify-machineinstrs %s -o /dev/null 2>&1 | FileCheck %s
; CHECK: failed to perform AVM tail call elimination on a call site marked musttail

declare i16 @f5(i16, i16, i16, i16, i16)
define i16 @unsupported(i16 %a, i16 %b, i16 %c, i16 %d, i16 %e) {
  %r = musttail call i16 @f5(i16 %a, i16 %b, i16 %c, i16 %d, i16 %e)
  ret i16 %r
}
