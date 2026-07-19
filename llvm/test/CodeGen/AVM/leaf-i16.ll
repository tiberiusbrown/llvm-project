; RUN: llc -mtriple=avm-unknown-arduboyfx -mcpu=avm1 \
; RUN:   -mtune=avm-interpreter-32u4-v1 -O2 -verify-machineinstrs < %s \
; RUN:   | FileCheck %s --check-prefix=ASM
; RUN: llc -mtriple=avm-unknown-arduboyfx -mcpu=avm1 \
; RUN:   -mtune=avm-interpreter-32u4-v1 -O2 -filetype=obj < %s -o %t.o
; RUN: llvm-objdump -d %t.o | FileCheck %s --check-prefix=OBJ
; RUN: not --crash llc -mtriple=avm-unknown-arduboyfx -mcpu=avm1 \
; RUN:   -mtune=not-an-avm-tune %s -o %t.bad 2>&1 \
; RUN:   | FileCheck %s --check-prefix=BAD-TUNE

define void @empty() {
; ASM-LABEL: empty:
; ASM-NEXT:  ; %bb.0:
; ASM-NEXT:  ret
  ret void
}

define i16 @add(i16 %a, i16 %b) {
; ASM-LABEL: add:
; ASM-NEXT:  ; %bb.0:
; ASM-NEXT:  add r4, r5
; ASM-NEXT:  ret
; OBJ-LABEL: <add>:
; OBJ-NEXT:  {{.*}}11{{[ ]+}}add r4, r5
; OBJ-NEXT:  {{.*}}ef{{[ ]+}}ret
  %v = add i16 %a, %b
  ret i16 %v
}

define i16 @sub(i16 %a, i16 %b) {
; ASM-LABEL: sub:
; ASM:       sub r4, r5
; ASM-NEXT:  ret
  %v = sub i16 %a, %b
  ret i16 %v
}

define i16 @and(i16 %a, i16 %b) {
; ASM-LABEL: and:
; ASM:       and r4, r5
; ASM-NEXT:  ret
  %v = and i16 %a, %b
  ret i16 %v
}

define i16 @or(i16 %a, i16 %b) {
; ASM-LABEL: or:
; ASM:       or r4, r5
; ASM-NEXT:  ret
  %v = or i16 %a, %b
  ret i16 %v
}

define i16 @xor(i16 %a, i16 %b) {
; ASM-LABEL: xor:
; ASM:       xor r4, r5
; ASM-NEXT:  ret
  %v = xor i16 %a, %b
  ret i16 %v
}

define i16 @constant8() {
; ASM-LABEL: constant8:
; ASM:       ldi8 r4, 42
; ASM-NEXT:  ret
  ret i16 42
}

define i16 @constant16() {
; ASM-LABEL: constant16:
; ASM:       ldi16 r4, 4660
; ASM-NEXT:  ret
  ret i16 4660
}

; BAD-TUNE: LLVM ERROR: unknown AVM tune CPU 'not-an-avm-tune'
