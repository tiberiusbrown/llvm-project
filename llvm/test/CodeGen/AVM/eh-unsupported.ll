; RUN: not llc -mtriple=avm-unknown-arduboyfx -O0 -filetype=asm %s -o /dev/null 2>&1 | FileCheck %s

; AVM supports typed catches, catch-all handlers, and cleanups. An exception filter
; has no AVM encoding, so it must fail the compile with its own diagnostic instead of
; producing an action list that misrepresents the handler. A typed catch compiles, as
; eh-typed.ll checks.

declare void @thrower() addrspace(1)
declare i32 @__gxx_personality_v0(...) addrspace(1)
@_ZTIi = external constant ptr

define i32 @typed() addrspace(1) personality ptr addrspace(1) @__gxx_personality_v0 {
  invoke addrspace(1) void @thrower() to label %normal unwind label %lpad

normal:
  ret i32 0

lpad:
  %pad = landingpad { ptr, i32 } catch ptr @_ZTIi
  ret i32 1
}

define i32 @filtered() addrspace(1) personality ptr addrspace(1) @__gxx_personality_v0 {
  invoke addrspace(1) void @thrower() to label %normal unwind label %lpad

normal:
  ret i32 0

lpad:
  %pad = landingpad { ptr, i32 } filter [1 x ptr] [ptr @_ZTIi]
  ret i32 1
}

; CHECK: error: AVM exceptions do not support exception filters in 'filtered'
; CHECK-NOT: 'typed'
