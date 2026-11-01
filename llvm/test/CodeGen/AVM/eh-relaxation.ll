; RUN: llc -mtriple=avm-unknown-arduboyfx -O2 -function-sections -filetype=obj %s -o %t.o
; RUN: ld.lld -flavor gnu --unresolved-symbols=ignore-all --entry=main -o %t.elf %t.o
; RUN: llvm-objdump -d %t.elf | FileCheck %s --check-prefix=CODE
; RUN: llvm-objdump -s -j .avm.eh_sites %t.elf | FileCheck %s --check-prefix=SITES
; RUN: llvm-readelf -r %t.o | FileCheck %s --check-prefix=RELOCS

; Relaxation changes the instruction sizes between the call sites in main, so
; every .avm.eh_sites endpoint must still match a linked instruction boundary.
; The functions are in separate sections, and the handlers are in blocks of
; their own. The callees sit next to main, so LLD relaxes each call to call8.

declare ptr @__cxa_allocate_exception(i16) addrspace(1)
declare void @__cxa_throw(ptr, ptr, ptr) addrspace(1)
declare ptr @__cxa_begin_catch(ptr) addrspace(1)
declare void @__cxa_end_catch() addrspace(1)
declare i32 @__gxx_personality_v0(...) addrspace(1)
@_ZTIi = external constant ptr

define void @near_leaf() addrspace(1) noinline {
  ret void
}

define void @throws(i16 %v) addrspace(1) noinline {
  %exc = call addrspace(1) ptr @__cxa_allocate_exception(i16 2)
  store i16 %v, ptr %exc, align 1
  call addrspace(1) void @__cxa_throw(ptr %exc, ptr @_ZTIi, ptr null)
  unreachable
}

define void @second_leaf() addrspace(1) noinline {
  ret void
}

define i32 @main(i16 %selector) addrspace(1) noinline personality ptr addrspace(1) @__gxx_personality_v0 {
entry:
  call addrspace(1) void @near_leaf()
  invoke addrspace(1) void @throws(i16 1) to label %after_first unwind label %pad_first

after_first:
  call addrspace(1) void @near_leaf()
  call addrspace(1) void @second_leaf()
  invoke addrspace(1) void @throws(i16 2) to label %after_second unwind label %pad_second

after_second:
  ret i32 0

pad_first:
  %first = landingpad { ptr, i32 } catch ptr null
  %first.exc = extractvalue { ptr, i32 } %first, 0
  %first.begin = call addrspace(1) ptr @__cxa_begin_catch(ptr %first.exc)
  call addrspace(1) void @__cxa_end_catch()
  br label %after_first

pad_second:
  %second = landingpad { ptr, i32 } catch ptr null
  %second.exc = extractvalue { ptr, i32 } %second, 0
  %second.begin = call addrspace(1) ptr @__cxa_begin_catch(ptr %second.exc)
  call addrspace(1) void @__cxa_end_catch()
  ret i32 1
}

; CODE-LABEL: <main>:
; CODE:       call8 {{.*}} <near_leaf>
; CODE:       call8 {{.*}} <throws>
; CODE:       call8 {{.*}} <near_leaf>
; CODE:       call8 {{.*}} <second_leaf>
; CODE:       call8 {{.*}} <throws>

; The record covers main from 0x113 to 0x136, with five call sites. The
; endpoints are linked addresses: 0x115, 0x119, 0x11d, and 0x121 follow the
; relaxed call8 instructions, and 0x12e and 0x124 are the two landing pads.
;   [0x113,0x115) near_leaf            no handler
;   [0x115,0x119) throws(1)            catch-all at 0x12e
;   [0x119,0x11d) near_leaf, second_leaf  no handler
;   [0x11d,0x121) throws(2)            catch-all at 0x124
;   [0x121,0x136) remainder of main    no handler
; SITES:      Contents of section .avm.eh_sites:
; SITES-NEXT: 0000 13010036 01000500 13010015 01000000
; SITES-NEXT: 0010 00001501 00190100 2e010001 1901001d
; SITES-NEXT: 0020 01000000 00001d01 00210100 24010001
; SITES-NEXT: 0030 21010036 01000000 0000

; RELOCS:     Relocation section '.rela.avm.eh_sites'
; RELOCS:     R_AVM_DEBUG24
