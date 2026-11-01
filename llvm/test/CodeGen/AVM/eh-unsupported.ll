; RUN: not llc -mtriple=avm-unknown-arduboyfx -O0 -filetype=asm %s -o /dev/null 2>&1 | FileCheck %s

; AVM supports catch-all handlers and cleanups. A typed catch and an exception
; filter have no AVM encoding, so each must fail the compile with its own
; diagnostic instead of producing call-site records that misrepresent the handler.

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

; CHECK-DAG: error: AVM exceptions support only catch-all handlers and cleanups; 'typed' has a typed catch
; CHECK-DAG: error: AVM exceptions support only catch-all handlers and cleanups; 'filtered' has an exception filter
