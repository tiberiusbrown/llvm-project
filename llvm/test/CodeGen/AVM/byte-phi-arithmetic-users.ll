; RUN: llc -mtriple=avm -O2 -start-before=avm-byte-phis -stop-after=avm-byte-phis %s -o - | FileCheck %s
; RUN: llc -mtriple=avm -O2 -verify-machineinstrs %s -o %t

; The byte-only PHI is pruned, but arithmetic feeding it still has a word
; consumer. Materialize that arithmetic even without a retained wide PHI.
define void @arithmetic_zext(ptr %p, ptr %out, i1 %choose) {
; CHECK-LABEL: define void @arithmetic_zext(
; CHECK: %loaded.word = zext i8 %loaded to i16
; CHECK: %added.word = add nuw i16 %loaded.word, 1
; CHECK: store volatile i16 %added.word, ptr %out
; CHECK: %byte = phi i8
; CHECK: store volatile i8 %byte, ptr %out
entry:
  %loaded = load volatile i8, ptr %p
  %added = add nuw i8 %loaded, 1
  %word = zext i8 %added to i16
  store volatile i16 %word, ptr %out
  br i1 %choose, label %join, label %other
other:
  %alternate = load volatile i8, ptr %p
  br label %join
join:
  %byte = phi i8 [%added, %entry], [%alternate, %other]
  store volatile i8 %byte, ptr %out
  ret void
}

; Switch users also need a materialized word, including when a chain of
; arithmetic survives and its former PHI consumer remains byte-sized.
define void @arithmetic_switch(ptr %p, ptr %out, i1 %choose) {
; CHECK-LABEL: define void @arithmetic_switch(
; CHECK: %loaded.word = zext i8 %loaded to i16
; CHECK: %added.word = add nuw i16 %loaded.word, 1
; CHECK: %subtracted.word = sub nuw i16 %added.word, 2
; CHECK: switch i16 %subtracted.word
; CHECK: i16 1, label %one
; CHECK: i16 255, label %high
; CHECK: %byte = phi i8
entry:
  %loaded = load volatile i8, ptr %p
  %added = add nuw i8 %loaded, 1
  %subtracted = sub nuw i8 %added, 2
  switch i8 %subtracted, label %other [i8 1, label %one
                                     i8 -1, label %high]
one:
  store volatile i16 1, ptr %out
  br label %select
high:
  store volatile i16 255, ptr %out
  br label %select
select:
  br i1 %choose, label %join, label %other
other:
  %alternate = load volatile i8, ptr %p
  br label %join
join:
  %byte = phi i8 [%subtracted, %select], [%alternate, %other]
  store volatile i8 %byte, ptr %out
  ret void
}
