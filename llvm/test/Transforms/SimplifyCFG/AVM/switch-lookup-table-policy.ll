; RUN: opt -passes='simplifycfg<switch-to-lookup>' -S %s | FileCheck %s --check-prefix=GENERIC
; RUN: opt -mtriple=avm-unknown-arduboyfx -mcpu=avm1 -mtune=avm-interpreter-32u4-v1 \
; RUN:   -passes='simplifycfg<switch-to-lookup>' -S %s | FileCheck %s --check-prefix=AVM

; SimplifyCFG builds a switch.table.* constant when the target allows lookup tables.
; AVM sets shouldBuildLookupTables to false, so the same dense switch stays a switch on
; AVM. The generic target builds the table, which shows the input qualifies.

target datalayout = "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-f80:128-n8:16:32:64-S128"

; GENERIC: @switch.table.test_1 = private unnamed_addr constant [4 x i8]
; AVM-NOT: switch.table

define i64 @test_1(i64 %0) {
; GENERIC-LABEL: define i64 @test_1(
; GENERIC:       switch.lookup:
; AVM-LABEL: define i64 @test_1(
; AVM:         switch i64 %2, label %
  %2 = urem i64 %0, 4
  switch i64 %2, label %5 [
  i64 1, label %3
  i64 2, label %3
  i64 3, label %4
  ]

3:
  br label %5

4:
  br label %5

5:
  %.0 = phi i64 [ 2, %4 ], [ 1, %3 ], [ 0, %1 ]
  ret i64 %.0
}
