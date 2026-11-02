; RUN: rm -rf %t && split-file %s %t

; Each module is well formed IR. llvm-as checks that before llc sees it. An operand
; that is neither null nor a global RTTI descriptor cannot be matched exactly on AVM,
; so llc must stop with a diagnostic. It must not take the operand as catch-all.
; Every case is a separate module, because the first fatal error ends the compile.

; RUN: llvm-as %t/inttoptr.ll -o /dev/null
; RUN: not llc -mtriple=avm-unknown-arduboyfx -O0 -filetype=asm %t/inttoptr.ll -o /dev/null 2>&1 | FileCheck %s --check-prefix=INTTOPTR
; RUN: llvm-as %t/gep.ll -o /dev/null
; RUN: not llc -mtriple=avm-unknown-arduboyfx -O0 -filetype=asm %t/gep.ll -o /dev/null 2>&1 | FileCheck %s --check-prefix=GEP
; RUN: llvm-as %t/function.ll -o /dev/null
; RUN: not llc -mtriple=avm-unknown-arduboyfx -O0 -filetype=asm %t/function.ll -o /dev/null 2>&1 | FileCheck %s --check-prefix=FUNCTION
; RUN: llvm-as %t/sentinel.ll -o /dev/null
; RUN: not llc -mtriple=avm-unknown-arduboyfx -O0 -filetype=asm %t/sentinel.ll -o /dev/null 2>&1 | FileCheck %s --check-prefix=SENTINEL
; RUN: llvm-as %t/undef.ll -o /dev/null
; RUN: not llc -mtriple=avm-unknown-arduboyfx -O0 -filetype=asm %t/undef.ll -o /dev/null 2>&1 | FileCheck %s --check-prefix=UNDEF
; RUN: llvm-as %t/catchpad.ll -o /dev/null
; RUN: not llc -mtriple=avm-unknown-arduboyfx -O0 -filetype=asm %t/catchpad.ll -o /dev/null 2>&1 | FileCheck %s --check-prefix=CATCHPAD
; RUN: llvm-as %t/typeid.ll -o /dev/null
; RUN: not llc -mtriple=avm-unknown-arduboyfx -O0 -filetype=asm %t/typeid.ll -o /dev/null 2>&1 | FileCheck %s --check-prefix=TYPEID
; RUN: llvm-as %t/alias-gep.ll -o /dev/null
; RUN: not llc -mtriple=avm-unknown-arduboyfx -O0 -filetype=asm %t/alias-gep.ll -o /dev/null 2>&1 | FileCheck %s --check-prefix=ALIASGEP
; RUN: llvm-as %t/alias-function.ll -o /dev/null
; RUN: not llc -mtriple=avm-unknown-arduboyfx -O0 -filetype=asm %t/alias-function.ll -o /dev/null 2>&1 | FileCheck %s --check-prefix=ALIASFUNCTION

; An inttoptr constant is not a descriptor, so it cannot be a typed catch.
; INTTOPTR: LLVM ERROR: AVM exception catch clause in 'bad_inttoptr' must reference a global RTTI descriptor or be null

; A nonzero-offset GEP into a descriptor names the wrong address, so it is rejected
; rather than stripped to the descriptor.
; GEP: LLVM ERROR: AVM exception catch clause in 'bad_gep' must reference a global RTTI descriptor or be null

; A function is a global, but it is never an RTTI descriptor.
; FUNCTION: LLVM ERROR: AVM exception catch clause in 'bad_function' must reference a global RTTI descriptor or be null

; llvm.eh.catch.all.value is LLVM's catch-all indirection. AVM takes the clause as
; catch-all only when it is null, so the indirection is rejected.
; SENTINEL: LLVM ERROR: AVM exception catch clause in 'bad_sentinel' must reference a global RTTI descriptor or be null

; An undef operand names no descriptor and is not null.
; UNDEF: LLVM ERROR: AVM exception catch clause in 'bad_undef' must reference a global RTTI descriptor or be null

; A catchpad is checked the same way as a landing-pad clause.
; CATCHPAD: LLVM ERROR: AVM exception catch clause in 'pad_inttoptr' must reference a global RTTI descriptor or be null

; llvm.eh.typeid.for is lowered to the same type ID table, so its operand is checked
; before it can become the catch-all ID.
; TYPEID: LLVM ERROR: AVM exception llvm.eh.typeid.for operand in 'bad_typeid' must reference a global RTTI descriptor or be null

; An alias may name a descriptor only directly or through other aliases. An alias whose
; aliasee is a constant expression, such as an offset into a descriptor, does not.
; ALIASGEP: LLVM ERROR: AVM exception catch clause in 'alias_gep' must reference a global RTTI descriptor or be null

; An alias of a function is an alias of a global, but never of a descriptor.
; ALIASFUNCTION: LLVM ERROR: AVM exception catch clause in 'alias_function' must reference a global RTTI descriptor or be null

;--- inttoptr.ll
declare void @thrower() addrspace(1)
declare i32 @__gxx_personality_v0(...) addrspace(1)

define i32 @bad_inttoptr() addrspace(1) personality ptr addrspace(1) @__gxx_personality_v0 {
  invoke addrspace(1) void @thrower() to label %normal unwind label %lpad

normal:
  ret i32 0

lpad:
  %pad = landingpad { ptr, i32 } catch ptr addrspace(1) inttoptr (i24 123 to ptr addrspace(1))
  ret i32 1
}

;--- gep.ll
declare void @thrower() addrspace(1)
declare i32 @__gxx_personality_v0(...) addrspace(1)
@_ZTIi = external addrspace(1) constant ptr addrspace(1)

define i32 @bad_gep() addrspace(1) personality ptr addrspace(1) @__gxx_personality_v0 {
  invoke addrspace(1) void @thrower() to label %normal unwind label %lpad

normal:
  ret i32 0

lpad:
  %pad = landingpad { ptr, i32 } catch ptr addrspace(1) getelementptr inbounds (i8, ptr addrspace(1) @_ZTIi, i16 4)
  ret i32 1
}

;--- function.ll
declare void @thrower() addrspace(1)
declare void @not_a_descriptor() addrspace(1)
declare i32 @__gxx_personality_v0(...) addrspace(1)

define i32 @bad_function() addrspace(1) personality ptr addrspace(1) @__gxx_personality_v0 {
  invoke addrspace(1) void @thrower() to label %normal unwind label %lpad

normal:
  ret i32 0

lpad:
  %pad = landingpad { ptr, i32 } catch ptr addrspace(1) @not_a_descriptor
  ret i32 1
}

;--- sentinel.ll
declare void @thrower() addrspace(1)
declare i32 @__gxx_personality_v0(...) addrspace(1)
@llvm.eh.catch.all.value = external addrspace(1) constant ptr addrspace(1)

define i32 @bad_sentinel() addrspace(1) personality ptr addrspace(1) @__gxx_personality_v0 {
  invoke addrspace(1) void @thrower() to label %normal unwind label %lpad

normal:
  ret i32 0

lpad:
  %pad = landingpad { ptr, i32 } catch ptr addrspace(1) @llvm.eh.catch.all.value
  ret i32 1
}

;--- undef.ll
declare void @thrower() addrspace(1)
declare i32 @__gxx_personality_v0(...) addrspace(1)

define i32 @bad_undef() addrspace(1) personality ptr addrspace(1) @__gxx_personality_v0 {
  invoke addrspace(1) void @thrower() to label %normal unwind label %lpad

normal:
  ret i32 0

lpad:
  %pad = landingpad { ptr, i32 } catch ptr undef
  ret i32 1
}

;--- catchpad.ll
declare void @thrower() addrspace(1)
declare i32 @__gxx_personality_v0(...) addrspace(1)

define void @pad_inttoptr() addrspace(1) personality ptr addrspace(1) @__gxx_personality_v0 {
entry:
  invoke addrspace(1) void @thrower() to label %normal unwind label %cs

normal:
  ret void

cs:
  %sw = catchswitch within none [label %pad] unwind to caller

pad:
  %cp = catchpad within %sw [ptr addrspace(1) inttoptr (i24 123 to ptr addrspace(1))]
  catchret from %cp to label %normal
}

;--- typeid.ll
declare void @thrower() addrspace(1)
declare i32 @__gxx_personality_v0(...) addrspace(1)
declare i32 @llvm.eh.typeid.for(ptr addrspace(1)) nounwind readnone
@_ZTIi = external addrspace(1) constant ptr addrspace(1)

define i32 @bad_typeid() addrspace(1) personality ptr addrspace(1) @__gxx_personality_v0 {
  invoke addrspace(1) void @thrower() to label %normal unwind label %lpad

normal:
  ret i32 0

lpad:
  %pad = landingpad { ptr, i32 } catch ptr addrspace(1) @_ZTIi
  %id = call i32 @llvm.eh.typeid.for(ptr addrspace(1) inttoptr (i24 123 to ptr addrspace(1)))
  ret i32 %id
}

;--- alias-gep.ll
declare void @thrower() addrspace(1)
declare i32 @__gxx_personality_v0(...) addrspace(1)
@_ZTIi_def = addrspace(1) constant ptr addrspace(1) null
@bad_alias = alias ptr addrspace(1), getelementptr (i8, ptr addrspace(1) @_ZTIi_def, i16 4)

define i32 @alias_gep() addrspace(1) personality ptr addrspace(1) @__gxx_personality_v0 {
  invoke addrspace(1) void @thrower() to label %normal unwind label %lpad

normal:
  ret i32 0

lpad:
  %pad = landingpad { ptr, i32 } catch ptr addrspace(1) @bad_alias
  ret i32 1
}

;--- alias-function.ll
declare void @thrower() addrspace(1)
declare i32 @__gxx_personality_v0(...) addrspace(1)

define void @the_function() addrspace(1) {
  ret void
}

@fn_alias = alias ptr addrspace(1), ptr addrspace(1) @the_function

define i32 @alias_function() addrspace(1) personality ptr addrspace(1) @__gxx_personality_v0 {
  invoke addrspace(1) void @thrower() to label %normal unwind label %lpad

normal:
  ret i32 0

lpad:
  %pad = landingpad { ptr, i32 } catch ptr addrspace(1) @fn_alias
  ret i32 1
}
