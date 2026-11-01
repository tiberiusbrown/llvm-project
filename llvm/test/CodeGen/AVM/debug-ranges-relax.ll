; RUN: llc -mtriple=avm-unknown-arduboyfx -O0 -filetype=obj %s -o %t.o
; RUN: llvm-readobj --relocations %t.o | FileCheck %s --check-prefix=RELOC
; RUN: llvm-dwarfdump --debug-ranges %t.o | FileCheck %s --check-prefix=RANGES

; The lexical block covers the "then" and "join" blocks, which are separated by
; the "else" block, so its DW_AT_ranges list has two entries. Relaxation can
; shorten the call and branch between the entries, so each address must be a
; relocated absolute reference. A difference from the CU base would be folded
; into a constant that the linker cannot update.

; RELOC: Section ({{.*}}) .rela.debug_ranges {
; RELOC-NEXT:   0x6 R_AVM_DEBUG24 .text 0x{{[0-9A-F]+}}
; RELOC-NEXT:   0x9 R_AVM_DEBUG24 .text 0x{{[0-9A-F]+}}
; RELOC-NEXT:   0xC R_AVM_DEBUG24 .text 0x{{[0-9A-F]+}}
; RELOC-NEXT:   0xF R_AVM_DEBUG24 .text 0x{{[0-9A-F]+}}

; RANGES: .debug_ranges contents:
; RANGES-NEXT: 00000000 ffffff 000000
; RANGES-NEXT: 00000000 {{[0-9a-f]+}} {{[0-9a-f]+}}
; RANGES-NEXT: 00000000 {{[0-9a-f]+}} {{[0-9a-f]+}}
; RANGES-NEXT: 00000000 <End of list>

declare void @sink(i8)
declare void @llvm.dbg.declare(metadata, metadata, metadata)

define void @scoped(i8 %x) !dbg !4 {
entry:
  %c = icmp eq i8 %x, 0
  %v = alloca i8, align 1
  br i1 %c, label %then, label %else, !dbg !10
then:
  call void @llvm.dbg.declare(metadata ptr %v, metadata !20, metadata !DIExpression()), !dbg !11
  call void @sink(i8 1), !dbg !11
  br label %join, !dbg !12
else:
  call void @sink(i8 2), !dbg !13
  br label %join, !dbg !13
join:
  call void @sink(i8 3), !dbg !11
  ret void, !dbg !14
}

!llvm.dbg.cu = !{!0}
!llvm.module.flags = !{!2, !3}

!0 = distinct !DICompileUnit(language: DW_LANG_C11, file: !1, producer: "avm test", isOptimized: false, runtimeVersion: 0, emissionKind: FullDebug)
!1 = !DIFile(filename: "scoped.c", directory: "/tmp")
!2 = !{i32 7, !"Dwarf Version", i32 4}
!3 = !{i32 2, !"Debug Info Version", i32 3}
!4 = distinct !DISubprogram(name: "scoped", scope: !1, file: !1, line: 1, type: !5, scopeLine: 1, spFlags: DISPFlagDefinition, unit: !0)
!5 = !DISubroutineType(types: !6)
!6 = !{null}
!7 = distinct !DILexicalBlock(scope: !4, file: !1, line: 2, column: 1)
!20 = !DILocalVariable(name: "v", scope: !7, file: !1, line: 2, type: !21)
!21 = !DIBasicType(name: "char", size: 8, encoding: DW_ATE_signed_char)
!10 = !DILocation(line: 2, column: 3, scope: !4)
!11 = !DILocation(line: 3, column: 5, scope: !7)
!12 = !DILocation(line: 4, column: 5, scope: !7)
!13 = !DILocation(line: 6, column: 5, scope: !4)
!14 = !DILocation(line: 8, column: 1, scope: !4)
