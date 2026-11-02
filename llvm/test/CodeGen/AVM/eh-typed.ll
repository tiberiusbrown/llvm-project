; RUN: llc -mtriple=avm-unknown-arduboyfx -O0 -filetype=asm %s -o - | FileCheck %s --check-prefix=ASM
; RUN: llc -mtriple=avm-unknown-arduboyfx -O2 -filetype=asm %s -o - | FileCheck %s --check-prefix=ASM-O2
; RUN: llc -mtriple=avm-unknown-arduboyfx -O0 -filetype=obj %s -o %t.o
; RUN: llvm-readelf -r %t.o | FileCheck %s --check-prefix=RELOCS
; RUN: llc -mtriple=avm-unknown-arduboyfx -O0 -filetype=asm %s -o - | not grep GCC_except_table

; Typed catches compile to ordered action lists in .avm.eh_sites. Each list is a
; flags byte, a catch count, and one (selector, RTTI address) entry per catch. The
; selector is the function's LLVM type ID, and a zero address is catch-all. LLVM
; appends the clauses of a landing pad to TypeIds in reverse source order, so the
; list is emitted in source order: the first clause is first, and catch-all is last.

declare void @thrower() addrspace(1)
declare void @destroy_local() addrspace(1)
declare ptr @__cxa_begin_catch(ptr) addrspace(1)
declare void @__cxa_end_catch() addrspace(1)
declare i32 @llvm.eh.typeid.for(ptr addrspace(1)) nounwind readnone
declare i32 @__gxx_personality_v0(...) addrspace(1)
@_ZTIi = external addrspace(1) constant ptr addrspace(1)
@_ZTIf = external addrspace(1) constant ptr addrspace(1)
@_ZTIj = external addrspace(1) constant ptr addrspace(1)
@_ZTIc = addrspace(1) constant ptr addrspace(1) null
@_ZTIc_alias = alias ptr addrspace(1), ptr addrspace(1) @_ZTIc
@_ZTIc_chain = alias ptr addrspace(1), ptr addrspace(1) @_ZTIc_alias

; Three clauses in source order: float, int, and catch-all. The catch-all is
; numbered first, because LLVM registers clauses in reverse, so the selectors
; are 3 for float, 2 for int, and 1 for catch-all.
define i32 @ordered() addrspace(1) personality ptr addrspace(1) @__gxx_personality_v0 {
  invoke addrspace(1) void @thrower() to label %normal unwind label %lpad

normal:
  ret i32 0

lpad:
  %pad = landingpad { ptr, i32 } catch ptr addrspace(1) @_ZTIf catch ptr addrspace(1) @_ZTIi catch ptr null
  %exc = extractvalue { ptr, i32 } %pad, 0
  %caught = call addrspace(1) ptr @__cxa_begin_catch(ptr %exc)
  call addrspace(1) void @__cxa_end_catch()
  ret i32 1
}

; The selector for int is the function's type ID for _ZTIi, and the typeid
; intrinsic lowers to the same constant that the action list records. Here int is
; numbered 1 and unsigned int is numbered 2, so the first clause in source order,
; unsigned int, is first in the list even though it has the larger ID.
define i32 @selector_of_int() addrspace(1) personality ptr addrspace(1) @__gxx_personality_v0 {
  invoke addrspace(1) void @thrower() to label %normal unwind label %lpad

normal:
  ret i32 0

lpad:
  %pad = landingpad { ptr, i32 } catch ptr addrspace(1) @_ZTIj catch ptr addrspace(1) @_ZTIi
  %id = call i32 @llvm.eh.typeid.for(ptr addrspace(1) @_ZTIi)
  %sel = extractvalue { ptr, i32 } %pad, 1
  %same = icmp eq i32 %sel, %id
  %matched = zext i1 %same to i32
  ret i32 %matched
}

; A cleanup with two typed catches. The pad keeps the cleanup flag, and the two
; catches are in source order.
define void @cleanup_and_typed() addrspace(1) personality ptr addrspace(1) @__gxx_personality_v0 {
  invoke addrspace(1) void @thrower() to label %normal unwind label %lpad

normal:
  ret void

lpad:
  %pad = landingpad { ptr, i32 } cleanup catch ptr addrspace(1) @_ZTIi catch ptr addrspace(1) @_ZTIf
  call addrspace(1) void @destroy_local()
  resume { ptr, i32 } %pad
}

; A cleanup-only pad has the flag and no catch entries.
define void @cleanup_only_pad() addrspace(1) personality ptr addrspace(1) @__gxx_personality_v0 {
  invoke addrspace(1) void @thrower() to label %normal unwind label %lpad

normal:
  ret void

lpad:
  %pad = landingpad { ptr, i32 } cleanup
  call addrspace(1) void @destroy_local()
  resume { ptr, i32 } %pad
}

; A catch-all only list has one entry, with a zero address.
define i32 @catch_all_only() addrspace(1) personality ptr addrspace(1) @__gxx_personality_v0 {
  invoke addrspace(1) void @thrower() to label %normal unwind label %lpad

normal:
  ret i32 0

lpad:
  %pad = landingpad { ptr, i32 } catch ptr null
  ret i32 1
}

; A typed clause after a catch-all can never run, so the list drops it. A clause
; that repeats an earlier one is unreachable too, and the list keeps the first.
define i32 @after_catch_all() addrspace(1) personality ptr addrspace(1) @__gxx_personality_v0 {
  invoke addrspace(1) void @thrower() to label %normal unwind label %lpad

normal:
  ret i32 0

lpad:
  %pad = landingpad { ptr, i32 } catch ptr addrspace(1) @_ZTIi catch ptr null catch ptr addrspace(1) @_ZTIf
  ret i32 1
}

define i32 @repeated_clause() addrspace(1) personality ptr addrspace(1) @__gxx_personality_v0 {
  invoke addrspace(1) void @thrower() to label %normal unwind label %lpad

normal:
  ret i32 0

lpad:
  %pad = landingpad { ptr, i32 } catch ptr addrspace(1) @_ZTIi catch ptr addrspace(1) @_ZTIi
  ret i32 1
}

; The supported pointer casts name their descriptor. An address-space cast of the
; float descriptor, a zero-index GEP into the int descriptor, and a null catch-all are
; all accepted, and their order is the clause order. The typeid of a zero-index GEP
; into the int descriptor is the ID of the int clause in this function.
define i32 @cast_operands() addrspace(1) personality ptr addrspace(1) @__gxx_personality_v0 {
  invoke addrspace(1) void @thrower() to label %normal unwind label %lpad

normal:
  ret i32 0

lpad:
  %pad = landingpad { ptr, i32 } catch ptr addrspacecast (ptr addrspace(1) @_ZTIf to ptr) catch ptr addrspace(1) getelementptr (i8, ptr addrspace(1) @_ZTIi, i16 0) catch ptr null
  %id = call i32 @llvm.eh.typeid.for(ptr addrspace(1) getelementptr (i8, ptr addrspace(1) @_ZTIi, i16 0))
  ret i32 %id
}

; An alias of a defined descriptor is a typed catch that names the alias symbol.
define i32 @alias_operand() addrspace(1) personality ptr addrspace(1) @__gxx_personality_v0 {
  invoke addrspace(1) void @thrower() to label %normal unwind label %lpad

normal:
  ret i32 0

lpad:
  %pad = landingpad { ptr, i32 } catch ptr addrspace(1) @_ZTIc_alias
  ret i32 1
}

; An alias of an alias of a defined descriptor is also a typed catch.
define i32 @alias_chain_operand() addrspace(1) personality ptr addrspace(1) @__gxx_personality_v0 {
  invoke addrspace(1) void @thrower() to label %normal unwind label %lpad

normal:
  ret i32 0

lpad:
  %pad = landingpad { ptr, i32 } catch ptr addrspace(1) @_ZTIc_chain
  ret i32 1
}

; The catch-all selector is the type ID of null, so a typeid of null in a function
; with a catch-all clause equals that clause's selector.
define i32 @selector_of_null() addrspace(1) personality ptr addrspace(1) @__gxx_personality_v0 {
  invoke addrspace(1) void @thrower() to label %normal unwind label %lpad

normal:
  ret i32 0

lpad:
  %pad = landingpad { ptr, i32 } catch ptr null
  %id = call i32 @llvm.eh.typeid.for(ptr addrspace(1) null)
  %sel = extractvalue { ptr, i32 } %pad, 1
  %same = icmp eq i32 %sel, %id
  %matched = zext i1 %same to i32
  ret i32 %matched
}

; ASM-LABEL: ordered:
; ASM:       .section .avm.eh_sites,"o",@progbits,.text
; ASM-NEXT:  .3byte .Lfunc_begin{{[0-9]+}}
; ASM-NEXT:  .3byte .Lfunc_end{{[0-9]+}}
; ASM-NEXT:  .short 2
; ASM-NEXT:  .short 18
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
; ASM-NEXT:  .short 3
; ASM-NEXT:  .short 3
; ASM-NEXT:  .3byte _ZTIf
; ASM-NEXT:  .short 2
; ASM-NEXT:  .3byte _ZTIi
; ASM-NEXT:  .short 1
; ASM-NEXT:  .short 0
; ASM-NEXT:  .byte 0

; The landing pad compares the selector in r5 with the typeid intrinsic's value,
; which is 1 for int in this function. The list holds unsigned int, then int with
; selector 1, so the compare and the action list name the same selector.
; ASM-LABEL: selector_of_int:
; ASM:       cmpi.s8 r5, 1
; ASM:       .section .avm.eh_sites,"o",@progbits,.text
; ASM-NEXT:  .3byte .Lfunc_begin{{[0-9]+}}
; ASM-NEXT:  .3byte .Lfunc_end{{[0-9]+}}
; ASM-NEXT:  .short 1
; ASM-NEXT:  .short 13
; ASM-NEXT:  .byte 0
; ASM-NEXT:  .3byte .Ltmp{{[0-9]+}}
; ASM-NEXT:  .3byte .Ltmp{{[0-9]+}}
; ASM-NEXT:  .3byte .Ltmp{{[0-9]+}}
; ASM-NEXT:  .short 1
; ASM-NEXT:  .byte 0
; ASM-NEXT:  .byte 0
; ASM-NEXT:  .short 2
; ASM-NEXT:  .short 2
; ASM-NEXT:  .3byte _ZTIj
; ASM-NEXT:  .short 1
; ASM-NEXT:  .3byte _ZTIi

; ASM-LABEL: cleanup_and_typed:
; ASM:       .section .avm.eh_sites,"o",@progbits,.text
; ASM-NEXT:  .3byte .Lfunc_begin{{[0-9]+}}
; ASM-NEXT:  .3byte .Lfunc_end{{[0-9]+}}
; ASM-NEXT:  .short 2
; ASM-NEXT:  .short 13
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
; ASM-NEXT:  .short 2
; ASM-NEXT:  .short 2
; ASM-NEXT:  .3byte _ZTIi
; ASM-NEXT:  .short 1
; ASM-NEXT:  .3byte _ZTIf

; A cleanup-only list has the flag and an empty catch list.
; ASM-LABEL: cleanup_only_pad:
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

; A catch-all only list has one entry with selector 1 and a zero address.
; ASM-LABEL: catch_all_only:
; ASM:       .section .avm.eh_sites,"o",@progbits,.text
; ASM-NEXT:  .3byte .Lfunc_begin{{[0-9]+}}
; ASM-NEXT:  .3byte .Lfunc_end{{[0-9]+}}
; ASM-NEXT:  .short 1
; ASM-NEXT:  .short 8
; ASM-NEXT:  .byte 0
; ASM-NEXT:  .3byte .Ltmp{{[0-9]+}}
; ASM-NEXT:  .3byte .Ltmp{{[0-9]+}}
; ASM-NEXT:  .3byte .Ltmp{{[0-9]+}}
; ASM-NEXT:  .short 1
; ASM-NEXT:  .byte 0
; ASM-NEXT:  .byte 0
; ASM-NEXT:  .short 1
; ASM-NEXT:  .short 1
; ASM-NEXT:  .short 0
; ASM-NEXT:  .byte 0

; ASM-LABEL: after_catch_all:
; ASM:       .section .avm.eh_sites,"o",@progbits,.text
; ASM-NEXT:  .3byte .Lfunc_begin{{[0-9]+}}
; ASM-NEXT:  .3byte .Lfunc_end{{[0-9]+}}
; ASM-NEXT:  .short 1
; ASM-NEXT:  .short 13
; ASM-NEXT:  .byte 0
; ASM-NEXT:  .3byte .Ltmp{{[0-9]+}}
; ASM-NEXT:  .3byte .Ltmp{{[0-9]+}}
; ASM-NEXT:  .3byte .Ltmp{{[0-9]+}}
; ASM-NEXT:  .short 1
; ASM-NEXT:  .byte 0
; ASM-NEXT:  .byte 0
; ASM-NEXT:  .short 2
; ASM-NEXT:  .short 3
; ASM-NEXT:  .3byte _ZTIi
; ASM-NEXT:  .short 2
; ASM-NEXT:  .short 0
; ASM-NEXT:  .byte 0

; ASM-LABEL: repeated_clause:
; ASM:       .section .avm.eh_sites,"o",@progbits,.text
; ASM-NEXT:  .3byte .Lfunc_begin{{[0-9]+}}
; ASM-NEXT:  .3byte .Lfunc_end{{[0-9]+}}
; ASM-NEXT:  .short 1
; ASM-NEXT:  .short 8
; ASM-NEXT:  .byte 0
; ASM-NEXT:  .3byte .Ltmp{{[0-9]+}}
; ASM-NEXT:  .3byte .Ltmp{{[0-9]+}}
; ASM-NEXT:  .3byte .Ltmp{{[0-9]+}}
; ASM-NEXT:  .short 1
; ASM-NEXT:  .byte 0
; ASM-NEXT:  .byte 0
; ASM-NEXT:  .short 1
; ASM-NEXT:  .short 1
; ASM-NEXT:  .3byte _ZTIi

; The cast operands produce the same list as their direct forms, in clause order.
; The typeid of the zero-index GEP into the int descriptor is the int clause's ID.
; ASM-LABEL: cast_operands:
; ASM:       ldi8 r4, 2
; ASM:       .section .avm.eh_sites,"o",@progbits,.text
; ASM-NEXT:  .3byte .Lfunc_begin{{[0-9]+}}
; ASM-NEXT:  .3byte .Lfunc_end{{[0-9]+}}
; ASM-NEXT:  .short 1
; ASM-NEXT:  .short 18
; ASM-NEXT:  .byte 0
; ASM-NEXT:  .3byte .Ltmp{{[0-9]+}}
; ASM-NEXT:  .3byte .Ltmp{{[0-9]+}}
; ASM-NEXT:  .3byte .Ltmp{{[0-9]+}}
; ASM-NEXT:  .short 1
; ASM-NEXT:  .byte 0
; ASM-NEXT:  .byte 0
; ASM-NEXT:  .short 3
; ASM-NEXT:  .short 3
; ASM-NEXT:  .3byte _ZTIf
; ASM-NEXT:  .short 2
; ASM-NEXT:  .3byte _ZTIi
; ASM-NEXT:  .short 1
; ASM-NEXT:  .short 0
; ASM-NEXT:  .byte 0

; An alias of a defined descriptor is a typed entry that names the alias symbol.
; ASM-LABEL: alias_operand:
; ASM:       .section .avm.eh_sites,"o",@progbits,.text
; ASM-NEXT:  .3byte .Lfunc_begin{{[0-9]+}}
; ASM-NEXT:  .3byte .Lfunc_end{{[0-9]+}}
; ASM-NEXT:  .short 1
; ASM-NEXT:  .short 8
; ASM-NEXT:  .byte 0
; ASM-NEXT:  .3byte .Ltmp{{[0-9]+}}
; ASM-NEXT:  .3byte .Ltmp{{[0-9]+}}
; ASM-NEXT:  .3byte .Ltmp{{[0-9]+}}
; ASM-NEXT:  .short 1
; ASM-NEXT:  .byte 0
; ASM-NEXT:  .byte 0
; ASM-NEXT:  .short 1
; ASM-NEXT:  .short 1
; ASM-NEXT:  .3byte _ZTIc_alias

; An alias chain is followed to the descriptor, and the entry names the chain's symbol.
; ASM-LABEL: alias_chain_operand:
; ASM:       .section .avm.eh_sites,"o",@progbits,.text
; ASM-NEXT:  .3byte .Lfunc_begin{{[0-9]+}}
; ASM-NEXT:  .3byte .Lfunc_end{{[0-9]+}}
; ASM-NEXT:  .short 1
; ASM-NEXT:  .short 8
; ASM-NEXT:  .byte 0
; ASM-NEXT:  .3byte .Ltmp{{[0-9]+}}
; ASM-NEXT:  .3byte .Ltmp{{[0-9]+}}
; ASM-NEXT:  .3byte .Ltmp{{[0-9]+}}
; ASM-NEXT:  .short 1
; ASM-NEXT:  .byte 0
; ASM-NEXT:  .byte 0
; ASM-NEXT:  .short 1
; ASM-NEXT:  .short 1
; ASM-NEXT:  .3byte _ZTIc_chain

; The typeid of null is the catch-all selector, 1 in this function, so the pad's
; compare is true.
; ASM-LABEL: selector_of_null:
; ASM:       cmpi.s8 r5, 1
; ASM-NEXT:  cset.eq r4

; ASM-O2: .section .avm.eh_sites
; ASM-O2: .3byte _ZTIf
; ASM-O2: .3byte _ZTIi

; The three RTTI addresses are relocated 24-bit references to the descriptors,
; and no LSDA is emitted.
; RELOCS:     Relocation section '.rela.avm.eh_sites'
; RELOCS:     R_AVM_DEBUG24 {{.*}} _ZTIf
; RELOCS:     R_AVM_DEBUG24 {{.*}} _ZTIi
; RELOCS:     R_AVM_DEBUG24 {{.*}} _ZTIj
