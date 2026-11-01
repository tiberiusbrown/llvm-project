; RUN: llc -mtriple=avm-unknown-arduboyfx -O0 -filetype=asm %s -o - | FileCheck %s --check-prefix=ASM
; RUN: llc -mtriple=avm-unknown-arduboyfx -O2 -filetype=asm %s -o - | FileCheck %s --check-prefix=ASM-O2
; RUN: llc -mtriple=avm-unknown-arduboyfx -O0 -filetype=obj %s -o %t.o

; Cleanup landing pads and combined cleanup-and-catch landing pads. The action
; byte in each .avm.eh_sites entry is a flag set: 1 is a catch-all handler, 2 is a
; cleanup, and a landing pad that cleans up and then catches sets both, which is 3.

declare void @thrower() addrspace(1)
declare void @destroy_local() addrspace(1)
declare ptr @__cxa_begin_catch(ptr) addrspace(1)
declare void @__cxa_end_catch() addrspace(1)
declare i32 @__gxx_personality_v0(...) addrspace(1)

; One invoke with a cleanup-only landing pad. The pad runs the cleanup and then
; resumes unwinding, so it records action 2 and no catch.
define void @cleanup_only() addrspace(1) personality ptr addrspace(1) @__gxx_personality_v0 {
  invoke addrspace(1) void @thrower() to label %normal unwind label %lpad

normal:
  ret void

lpad:
  %pad = landingpad { ptr, i32 } cleanup
  call addrspace(1) void @destroy_local()
  resume { ptr, i32 } %pad
}

; Two invokes with two cleanup-only pads. Each pad records its own action 2 entry.
define void @two_cleanups() addrspace(1) personality ptr addrspace(1) @__gxx_personality_v0 {
  invoke addrspace(1) void @thrower() to label %second unwind label %first_pad

second:
  invoke addrspace(1) void @thrower() to label %normal unwind label %second_pad

normal:
  ret void

first_pad:
  %first = landingpad { ptr, i32 } cleanup
  call addrspace(1) void @destroy_local()
  resume { ptr, i32 } %first

second_pad:
  %second_lp = landingpad { ptr, i32 } cleanup
  call addrspace(1) void @destroy_local()
  resume { ptr, i32 } %second_lp
}

; A landing pad with a cleanup clause and a catch-all clause. The cleanup runs
; first, and then the catch-all handler takes the exception, so the pad records
; action 3. Its control flow never reaches resume.
define i32 @cleanup_and_catch() addrspace(1) personality ptr addrspace(1) @__gxx_personality_v0 {
  invoke addrspace(1) void @thrower() to label %normal unwind label %lpad

normal:
  ret i32 0

lpad:
  %pad = landingpad { ptr, i32 } cleanup catch ptr null
  %exc = extractvalue { ptr, i32 } %pad, 0
  call addrspace(1) void @destroy_local()
  %caught = call addrspace(1) ptr @__cxa_begin_catch(ptr %exc)
  call addrspace(1) void @__cxa_end_catch()
  ret i32 1
}

; The catch-only pad keeps action 1, so the three actions appear together.
define i32 @catch_only() addrspace(1) personality ptr addrspace(1) @__gxx_personality_v0 {
  invoke addrspace(1) void @thrower() to label %normal unwind label %lpad

normal:
  ret i32 0

lpad:
  %pad = landingpad { ptr, i32 } catch ptr null
  %exc = extractvalue { ptr, i32 } %pad, 0
  %caught = call addrspace(1) ptr @__cxa_begin_catch(ptr %exc)
  call addrspace(1) void @__cxa_end_catch()
  ret i32 2
}

; Each site record is u24 start, u24 end, u24 landing pad, and the action byte. The
; call-site table also has a record for the gap after the invoke, so the count is
; not the number of landing pads.
; ASM-LABEL: cleanup_only:
; ASM:       .section .avm.eh_sites
; ASM-NEXT:  .3byte .Lfunc_begin{{[0-9]+}}
; ASM-NEXT:  .3byte .Lfunc_end{{[0-9]+}}
; ASM-NEXT:  .short 2
; ASM-NEXT:  .3byte .Ltmp{{[0-9]+}}
; ASM-NEXT:  .3byte .Ltmp{{[0-9]+}}
; ASM-NEXT:  .3byte .Ltmp{{[0-9]+}}
; ASM-NEXT:  .byte 2
;
; Each cleanup pad keeps its own record with action 2.
; ASM-LABEL: two_cleanups:
; ASM:       .section .avm.eh_sites
; ASM-NEXT:  .3byte .Lfunc_begin{{[0-9]+}}
; ASM-NEXT:  .3byte .Lfunc_end{{[0-9]+}}
; ASM-NEXT:  .short 3
; ASM-NEXT:  .3byte .Ltmp{{[0-9]+}}
; ASM-NEXT:  .3byte .Ltmp{{[0-9]+}}
; ASM-NEXT:  .3byte .Ltmp{{[0-9]+}}
; ASM-NEXT:  .byte 2
; ASM-NEXT:  .3byte .Ltmp{{[0-9]+}}
; ASM-NEXT:  .3byte .Ltmp{{[0-9]+}}
; ASM-NEXT:  .3byte .Ltmp{{[0-9]+}}
; ASM-NEXT:  .byte 2
;
; The combined pad records action 3: the cleanup flag and the catch-all flag.
; ASM-LABEL: cleanup_and_catch:
; ASM:       .section .avm.eh_sites
; ASM-NEXT:  .3byte .Lfunc_begin{{[0-9]+}}
; ASM-NEXT:  .3byte .Lfunc_end{{[0-9]+}}
; ASM-NEXT:  .short 2
; ASM-NEXT:  .3byte .Ltmp{{[0-9]+}}
; ASM-NEXT:  .3byte .Ltmp{{[0-9]+}}
; ASM-NEXT:  .3byte .Ltmp{{[0-9]+}}
; ASM-NEXT:  .byte 3
;
; The catch-only pad keeps action 1.
; ASM-LABEL: catch_only:
; ASM:       .section .avm.eh_sites
; ASM-NEXT:  .3byte .Lfunc_begin{{[0-9]+}}
; ASM-NEXT:  .3byte .Lfunc_end{{[0-9]+}}
; ASM-NEXT:  .short 2
; ASM-NEXT:  .3byte .Ltmp{{[0-9]+}}
; ASM-NEXT:  .3byte .Ltmp{{[0-9]+}}
; ASM-NEXT:  .3byte .Ltmp{{[0-9]+}}
; ASM-NEXT:  .byte 1

; The cleanup-only pads keep action 2 at -O2. The optimizer drops the cleanup clause
; from a landing pad that has a catch-all clause, so the combined pad records action 1
; at -O2, as clang's optimized builds do.
; ASM-O2-LABEL: cleanup_only:
; ASM-O2:       .section .avm.eh_sites
; ASM-O2:       .byte 2
; ASM-O2-LABEL: two_cleanups:
; ASM-O2:       .section .avm.eh_sites
; ASM-O2:       .byte 2
; ASM-O2:       .byte 2
; ASM-O2-LABEL: cleanup_and_catch:
; ASM-O2:       .section .avm.eh_sites
; ASM-O2:       .byte 1
; ASM-O2-LABEL: catch_only:
; ASM-O2:       .section .avm.eh_sites
; ASM-O2:       .byte 1
