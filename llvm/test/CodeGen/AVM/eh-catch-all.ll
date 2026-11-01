; RUN: llc -mtriple=avm-unknown-arduboyfx -O0 -filetype=asm %s -o - | FileCheck %s --check-prefix=ASM
; RUN: llc -mtriple=avm-unknown-arduboyfx -O2 -filetype=asm %s -o - | FileCheck %s --check-prefix=ASM-O2
; RUN: llc -mtriple=avm-unknown-arduboyfx -O0 -filetype=obj %s -o %t.o
; RUN: llvm-readelf -S %t.o | FileCheck %s --check-prefix=SECTIONS
; RUN: llvm-readelf -r %t.o | FileCheck %s --check-prefix=RELOCS

; A catch-all handler that receives its exception object in r4 and its selector
; in r5. The call to outer() is an invoke with a landing pad, and the call-site
; records use the one-byte catch-all action.

declare ptr @__cxa_allocate_exception(i16) addrspace(1)
declare void @__cxa_throw(ptr, ptr, ptr) addrspace(1)
declare ptr @__cxa_begin_catch(ptr) addrspace(1)
declare void @__cxa_end_catch() addrspace(1)
declare i32 @__gxx_personality_v0(...) addrspace(1)
@_ZTIi = external constant ptr

define void @inner() addrspace(1) noinline {
  %exc = call addrspace(1) ptr @__cxa_allocate_exception(i16 2)
  store i16 42, ptr %exc, align 1
  call addrspace(1) void @__cxa_throw(ptr %exc, ptr @_ZTIi, ptr null)
  unreachable
}

define void @outer() addrspace(1) noinline {
  call addrspace(1) void @inner()
  ret void
}

define i32 @main() addrspace(1) noinline personality ptr addrspace(1) @__gxx_personality_v0 {
  invoke addrspace(1) void @outer() to label %normal unwind label %lpad

normal:
  ret i32 1

lpad:
  %pad = landingpad { ptr, i32 } catch ptr null
  %exc = extractvalue { ptr, i32 } %pad, 0
  %sel = extractvalue { ptr, i32 } %pad, 1
  %caught = call addrspace(1) ptr @__cxa_begin_catch(ptr %exc)
  call addrspace(1) void @__cxa_end_catch()
  ret i32 %sel
}

; ASM-LABEL: main:
; ASM:       .cfi_personality 0, __gxx_personality_v0
; ASM:       .Ltmp0: ; EH_LABEL
; ASM-NEXT:  call outer
; ASM-NEXT:  .Leh_call{{[0-9]+}}:
; ASM-NEXT:  .Ltmp1: ; EH_LABEL
;
; The landing pad receives the exception header in r4 and the catch selector
; in r5. r4 reaches __cxa_begin_catch unchanged. The selector is copied out of
; r5, and its upper half is cleared to form the zero-extended i32.
; ASM:       .Ltmp2: ; EH_LABEL
; ASM-NEXT:  mov r6, r5
; ASM-NEXT:  xor r7, r7
; ASM:       call __cxa_begin_catch
; ASM:       call __cxa_end_catch
;
; Each ordinary call in a function with landing pads gets a return label and an
; .avm.eh_calls record. The record holds the function start, the call count, and
; the return label with the outgoing argument bytes of each call.
; ASM:       .section .avm.eh_calls,"o",@progbits,.text
; ASM-NEXT:  .3byte .Lfunc_begin{{[0-9]+}}
; ASM-NEXT:  .short {{[1-9][0-9]*}}
; ASM:       .3byte .Leh_call{{[0-9]+}}
; ASM-NEXT:  .short 0

; The call-site records are emitted into .avm.eh_sites, and no LSDA section is
; generated.
; ASM:       .section .avm.eh_sites,"o",@progbits,.text
; ASM-NEXT:  .3byte .Lfunc_begin{{[0-9]+}}
; ASM-NEXT:  .3byte .Lfunc_end{{[0-9]+}}
; ASM-NEXT:  .short 2
; ASM-NEXT:  .3byte .Ltmp{{[0-9]+}}
; ASM-NEXT:  .3byte .Ltmp{{[0-9]+}}
; ASM-NEXT:  .3byte .Ltmp{{[0-9]+}}
; ASM-NEXT:  .byte 1
; ASM-NOT:   .gcc_except_table
; ASM-NOT:   .eh_frame
;
; ASM-O2-LABEL: main:
; ASM-O2:       .Ltmp{{[0-9]+}}: ; EH_LABEL
; ASM-O2:       call outer
; ASM-O2:       .section .avm.eh_sites
; ASM-O2-NOT:   .gcc_except_table
; ASM-O2-NOT:   .eh_frame


; The frame and site records are nonallocated, and no EH frame or LSDA section
; is produced.
; SECTIONS:       .avm.eh_calls PROGBITS
; SECTIONS:       .avm.eh_sites PROGBITS
; SECTIONS:       .rela.avm.eh_sites RELA
; SECTIONS:       .debug_frame PROGBITS
; SECTIONS-NOT:   .eh_frame
; SECTIONS-NOT:   .gcc_except_table
;
; Each site address is an absolute three-byte reference to a label in the
; function, so the linker can adjust it after relaxation.
; RELOCS:         Relocation section '.rela.avm.eh_sites'
; RELOCS:         R_AVM_DEBUG24
