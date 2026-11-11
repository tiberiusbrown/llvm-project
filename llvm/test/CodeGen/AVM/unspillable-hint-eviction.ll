; RUN: llc -mtriple=avm -O2 -verify-machineinstrs %s -o - | FileCheck %s
; RUN: not llc -mtriple=avm -O2 -stress-regalloc=1 %s -o /dev/null 2>&1 | FileCheck %s --check-prefix=STRESS

; The llvm.avm.load result must stay in r4, its only register. Its live range is
; a single instruction, so it is unspillable. A hint-driven eviction must not
; remove it: the evicted range has nowhere else to go, so allocation fails with
; "ran out of registers".

; CHECK-LABEL: _Z9game_loadv:
; CHECK:       sys load

; STRESS: error: <unknown>:0:0: ran out of registers during register allocation in function '_Z9game_loadv'
; STRESS-NOT: Stack dump

define i1 @_Z9game_loadv(i8 %0, i1 %cmp1.not.i, ptr %1, i8 %2, i1 %or.cond, i1 %cmp5.not.i, ptr addrspace(1) %OUTDOOR_AREA, i24 %residence.sroa.3.sroa.0.0.insert.ext.i) addrspace(1) {
entry:
  %3 = tail call addrspace(1) i16 @llvm.avm.load()
  br i1 %cmp5.not.i, label %land.lhs.true.i, label %land.end

land.lhs.true.i:                                  ; preds = %entry
  br i1 %cmp1.not.i, label %cleanup141.i, label %land.lhs.true.i.i

land.lhs.true.i.i:                                ; preds = %land.lhs.true.i
  store i24 %residence.sroa.3.sroa.0.0.insert.ext.i, ptr %1, align 1
  ret i1 false

cleanup141.i:                                     ; preds = %land.lhs.true.i
  ret i1 false

land.end:                                         ; preds = %entry
  ret i1 %or.cond
}

; Function Attrs: nocallback nofree nounwind willreturn memory(write)
declare i16 @llvm.avm.load() addrspace(1) #0

attributes #0 = { nocallback nofree nounwind willreturn memory(write) }
