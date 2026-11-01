; RUN: llc -mtriple=avm-unknown-arduboyfx -O2 -filetype=obj %s -o %t.o
; RUN: llvm-objdump -s -j .debug_loc %t.o | FileCheck %s --check-prefix=BYTES
; RUN: llvm-readobj --relocations %t.o | FileCheck %s --check-prefix=RELOC
; RUN: llvm-dwarfdump --debug-loc %t.o | FileCheck %s --check-prefix=LOC

; The only function in the module gives its compile unit a single range, so the
; unit's DW_AT_low_pc is nonzero and each DWARF 4 location list starts from that
; base. The list must select a zero base first, which makes its three-byte
; entries absolute. Each endpoint is a relocated absolute reference, never a
; difference of labels in .text, because relaxation shortens the call between the
; list entries and a folded constant would not be updated by the linker.

; BYTES: Contents of section .debug_loc:
; BYTES-NEXT: 0000 ffffff00 0000

; RELOC: Section ({{.*}}) .rela.debug_loc {
; RELOC-NEXT: 0x6 R_AVM_DEBUG24 .text 0x0
; RELOC-NEXT: 0x9 R_AVM_DEBUG24 .text 0x3
; RELOC-NEXT: 0xF R_AVM_DEBUG24 .text 0x3
; RELOC-NEXT: 0x12 R_AVM_DEBUG24 .text 0xD
; RELOC-NEXT: 0x18 R_AVM_DEBUG24 .text 0xD
; RELOC-NEXT: 0x1B R_AVM_DEBUG24 .text 0xF
; RELOC-NEXT: 0x21 R_AVM_DEBUG24 .text 0x13
; RELOC-NEXT: 0x24 R_AVM_DEBUG24 .text 0x1A

; LOC: .debug_loc contents:
; LOC-NEXT: 0x00000000:
; LOC-NEXT: [0x000000, 0x000003): DW_OP_reg4 R4
; LOC-NEXT: [0x000003, 0x00000d): DW_OP_reg0 R0
; LOC-NEXT: [0x00000d, 0x00000f): DW_OP_reg4 R4
; LOC-NEXT: [0x000013, 0x00001a): DW_OP_reg0 R0

declare void @sink(i8)
declare void @llvm.dbg.value(metadata, metadata, metadata)

define void @scoped(i8 %x) !dbg !4 {
entry:
  call void @llvm.dbg.value(metadata i8 %x, metadata !20, metadata !DIExpression()), !dbg !10
  call void @sink(i8 1), !dbg !11
  %y = add i8 %x, 1
  call void @llvm.dbg.value(metadata i8 %y, metadata !20, metadata !DIExpression()), !dbg !12
  call void @sink(i8 %y), !dbg !13
  call void @llvm.dbg.value(metadata i8 %x, metadata !20, metadata !DIExpression()), !dbg !12
  call void @sink(i8 %x), !dbg !14
  ret void, !dbg !15
}

!llvm.dbg.cu = !{!0}
!llvm.module.flags = !{!2, !3}

!0 = distinct !DICompileUnit(language: DW_LANG_C11, file: !1, producer: "avm test", isOptimized: true, runtimeVersion: 0, emissionKind: FullDebug)
!1 = !DIFile(filename: "loc.c", directory: "/tmp")
!2 = !{i32 7, !"Dwarf Version", i32 4}
!3 = !{i32 2, !"Debug Info Version", i32 3}
!4 = distinct !DISubprogram(name: "scoped", scope: !1, file: !1, line: 1, type: !5, scopeLine: 1, spFlags: DISPFlagDefinition | DISPFlagOptimized, unit: !0)
!5 = !DISubroutineType(types: !6)
!6 = !{null}
!10 = !DILocation(line: 2, column: 3, scope: !4)
!11 = !DILocation(line: 3, column: 5, scope: !4)
!12 = !DILocation(line: 4, column: 5, scope: !4)
!13 = !DILocation(line: 5, column: 5, scope: !4)
!14 = !DILocation(line: 6, column: 5, scope: !4)
!15 = !DILocation(line: 8, column: 1, scope: !4)
!20 = !DILocalVariable(name: "x", arg: 1, scope: !4, file: !1, line: 1, type: !21)
!21 = !DIBasicType(name: "char", size: 8, encoding: DW_ATE_signed_char)
