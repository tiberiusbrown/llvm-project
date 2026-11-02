; RUN: llc -mtriple=avm-unknown-arduboyfx -O0 -filetype=asm %s -o - | FileCheck %s --check-prefix=ASM
; RUN: llc -mtriple=avm-unknown-arduboyfx -O2 -filetype=asm %s -o - | FileCheck %s --check-prefix=ASM-O2
; RUN: llc -mtriple=avm-unknown-arduboyfx -O0 -filetype=obj %s -o %t.o

; Cleanup landing pads and combined cleanup-and-catch landing pads. Each action list
; in the pool of .avm.eh_sites has a flags byte: bit 0 is a cleanup. A catch-all is a
; catch entry with a zero type address, so it is not a flag.

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

; The invoke's site and the gap after it. The pool holds one list: flags 1 for the
; cleanup, and no catch entries.
; ASM-LABEL: cleanup_only:
; ASM:       .section .avm.eh_sites,"o",@progbits,.text
; ASM-NEXT:  .3byte .Lfunc_begin{{[0-9]+}}
; ASM-NEXT:  .3byte .Lfunc_end{{[0-9]+}}
; ASM-NEXT:  .short 2
; ASM-NEXT:  .short 3
; ASM-NEXT:  .byte 0
; ASM-NEXT:  .3byte .Ltmp{{[0-9]+}}
; ASM-NEXT:  .3byte .Ltmp{{[0-9]+}}
; ASM-NEXT:  .3byte .Ltmp{{[0-9]+}}
; ASM-NEXT:  .short 1
; ASM-NEXT:  .byte 0
; ASM-NEXT:  .3byte .Ltmp{{[0-9]+}}
; ASM-NEXT:  .3byte .Lfunc_end{{[0-9]+}}
; ASM-NEXT:  .short 0
; ASM-NEXT:  .byte 0
; ASM-NEXT:  .short 0
; ASM-NEXT:  .byte 0
; ASM-NEXT:  .byte 1
; ASM-NEXT:  .short 0
;
; Two pads with the same cleanup share one list, so both sites name the same offset.
; ASM-LABEL: two_cleanups:
; ASM:       .section .avm.eh_sites,"o",@progbits,.text
; ASM-NEXT:  .3byte .Lfunc_begin{{[0-9]+}}
; ASM-NEXT:  .3byte .Lfunc_end{{[0-9]+}}
; ASM-NEXT:  .short 3
; ASM-NEXT:  .short 3
; ASM-NEXT:  .byte 0
; ASM-NEXT:  .3byte .Ltmp{{[0-9]+}}
; ASM-NEXT:  .3byte .Ltmp{{[0-9]+}}
; ASM-NEXT:  .3byte .Ltmp{{[0-9]+}}
; ASM-NEXT:  .short 1
; ASM-NEXT:  .byte 0
; ASM-NEXT:  .3byte .Ltmp{{[0-9]+}}
; ASM-NEXT:  .3byte .Ltmp{{[0-9]+}}
; ASM-NEXT:  .3byte .Ltmp{{[0-9]+}}
; ASM-NEXT:  .short 1
; ASM-NEXT:  .byte 0
; ASM-NEXT:  .3byte .Ltmp{{[0-9]+}}
; ASM-NEXT:  .3byte .Lfunc_end{{[0-9]+}}
; ASM-NEXT:  .short 0
; ASM-NEXT:  .byte 0
; ASM-NEXT:  .short 0
; ASM-NEXT:  .byte 0
; ASM-NEXT:  .byte 1
; ASM-NEXT:  .short 0
;
; The combined pad records a cleanup flag and a catch-all entry: flags 1, one
; entry with selector 1 and a zero address. At -O2, the optimizer drops the cleanup
; clause from a landing pad that has a catch-all clause, so the flag is then zero.
; ASM-LABEL: cleanup_and_catch:
; ASM:       .section .avm.eh_sites,"o",@progbits,.text
; ASM-NEXT:  .3byte .Lfunc_begin{{[0-9]+}}
; ASM-NEXT:  .3byte .Lfunc_end{{[0-9]+}}
; ASM-NEXT:  .short 2
; ASM-NEXT:  .short 8
; ASM-NEXT:  .byte 0
; ASM-NEXT:  .3byte .Ltmp{{[0-9]+}}
; ASM-NEXT:  .3byte .Ltmp{{[0-9]+}}
; ASM-NEXT:  .3byte .Ltmp{{[0-9]+}}
; ASM-NEXT:  .short 1
; ASM-NEXT:  .byte 0
; ASM-NEXT:  .3byte .Ltmp{{[0-9]+}}
; ASM-NEXT:  .3byte .Lfunc_end{{[0-9]+}}
; ASM-NEXT:  .short 0
; ASM-NEXT:  .byte 0
; ASM-NEXT:  .short 0
; ASM-NEXT:  .byte 0
; ASM-NEXT:  .byte 1
; ASM-NEXT:  .short 1
; ASM-NEXT:  .short 1
; ASM-NEXT:  .short 0
; ASM-NEXT:  .byte 0

; A catch-only pad has no cleanup flag, and its one entry is the catch-all.
; ASM-LABEL: catch_only:
; ASM:       .section .avm.eh_sites,"o",@progbits,.text
; ASM-NEXT:  .3byte .Lfunc_begin{{[0-9]+}}
; ASM-NEXT:  .3byte .Lfunc_end{{[0-9]+}}
; ASM-NEXT:  .short 2
; ASM-NEXT:  .short 8
; ASM-NEXT:  .byte 0
; ASM-NEXT:  .3byte .Ltmp{{[0-9]+}}
; ASM-NEXT:  .3byte .Ltmp{{[0-9]+}}
; ASM-NEXT:  .3byte .Ltmp{{[0-9]+}}
; ASM-NEXT:  .short 1
; ASM-NEXT:  .byte 0
; ASM-NEXT:  .3byte .Ltmp{{[0-9]+}}
; ASM-NEXT:  .3byte .Lfunc_end{{[0-9]+}}
; ASM-NEXT:  .short 0
; ASM-NEXT:  .byte 0
; ASM-NEXT:  .short 0
; ASM-NEXT:  .byte 0
; ASM-NEXT:  .byte 0
; ASM-NEXT:  .short 1
; ASM-NEXT:  .short 1
; ASM-NEXT:  .short 0
; ASM-NEXT:  .byte 0

; The cleanup-only pads keep the cleanup flag at -O2. The combined pad drops it, as
; the -O0 checks above show.
; ASM-O2-LABEL: cleanup_only:
; ASM-O2:       .section .avm.eh_sites
; ASM-O2:       .byte 1
; ASM-O2-NEXT:  .short 0
; ASM-O2-LABEL: two_cleanups:
; ASM-O2:       .section .avm.eh_sites
; ASM-O2:       .byte 1
; ASM-O2-NEXT:  .short 0
; ASM-O2-LABEL: cleanup_and_catch:
; ASM-O2:       .section .avm.eh_sites
; ASM-O2-NEXT:  .3byte .Lfunc_begin{{[0-9]+}}
; ASM-O2-NEXT:  .3byte .Lfunc_end{{[0-9]+}}
; ASM-O2-NEXT:  .short 2
; ASM-O2-NEXT:  .short 8
; ASM-O2-NEXT:  .byte 0
; ASM-O2-NEXT:  .3byte .Ltmp{{[0-9]+}}
; ASM-O2-NEXT:  .3byte .Ltmp{{[0-9]+}}
; ASM-O2-NEXT:  .3byte .Ltmp{{[0-9]+}}
; ASM-O2-NEXT:  .short 1
; ASM-O2-NEXT:  .byte 0
; ASM-O2-NEXT:  .3byte .Ltmp{{[0-9]+}}
; ASM-O2-NEXT:  .3byte .Lfunc_end{{[0-9]+}}
; ASM-O2-NEXT:  .short 0
; ASM-O2-NEXT:  .byte 0
; ASM-O2-NEXT:  .short 0
; ASM-O2-NEXT:  .byte 0
; ASM-O2-NEXT:  .byte 0
; ASM-O2-NEXT:  .short 1
; ASM-O2-NEXT:  .short 1
; ASM-O2-NEXT:  .short 0
; ASM-O2-NEXT:  .byte 0
; ASM-O2-LABEL: catch_only:
; ASM-O2:       .section .avm.eh_sites
; ASM-O2-NEXT:  .3byte .Lfunc_begin{{[0-9]+}}
; ASM-O2-NEXT:  .3byte .Lfunc_end{{[0-9]+}}
; ASM-O2-NEXT:  .short 2
; ASM-O2-NEXT:  .short 8
; ASM-O2-NEXT:  .byte 0
; ASM-O2-NEXT:  .3byte .Ltmp{{[0-9]+}}
; ASM-O2-NEXT:  .3byte .Ltmp{{[0-9]+}}
; ASM-O2-NEXT:  .3byte .Ltmp{{[0-9]+}}
; ASM-O2-NEXT:  .short 1
; ASM-O2-NEXT:  .byte 0
; ASM-O2-NEXT:  .3byte .Ltmp{{[0-9]+}}
; ASM-O2-NEXT:  .3byte .Lfunc_end{{[0-9]+}}
; ASM-O2-NEXT:  .short 0
; ASM-O2-NEXT:  .byte 0
; ASM-O2-NEXT:  .short 0
; ASM-O2-NEXT:  .byte 0
; ASM-O2-NEXT:  .byte 0
; ASM-O2-NEXT:  .short 1
; ASM-O2-NEXT:  .short 1
; ASM-O2-NEXT:  .short 0
; ASM-O2-NEXT:  .byte 0
