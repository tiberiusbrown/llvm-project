; RUN: not --crash llc -mtriple=avm-unknown-arduboyfx %s -o - 2>&1 | FileCheck %s

; Calls are deliberately outside the first code-generation slice. Verify that
; they fail in GlobalISel instead of silently producing incorrect code.

declare i16 @callee(i16)

define i16 @unsupported_call(i16 %value) {
  %result = call i16 @callee(i16 %value)
  ret i16 %result
}

; CHECK: unable to translate instruction: call
