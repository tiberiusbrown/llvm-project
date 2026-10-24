; RUN: llc -mtriple=avm -O2 -verify-machineinstrs -print-after=avm-constant-initializers %s -o %t 2>&1 | FileCheck %s --check-prefix=IR
; RUN: FileCheck %s --check-prefix=ASM < %t
; RUN: llc -mtriple=avm -O0 -verify-machineinstrs %s -o - | FileCheck %s --check-prefix=O0 --implicit-check-not="sys memcpy_p"
; RUN: llc -mtriple=avm -O2 -avm-disable-constant-initializers %s -o - | FileCheck %s --check-prefix=O0 --implicit-check-not="sys memcpy_p"

; RUN: llc -mtriple=avm -O2 -stop-after=finalize-isel -verify-machineinstrs %s -o - | FileCheck %s --check-prefix=ISEL
; ISEL-LABEL: name: words
; ISEL: hasCalls: false
; ISEL: CONST_COPY_P_PSEUDO $r4, @.avm.init, 64, csr_avm_call, implicit-def dead $cc :: (store (s512) into %ir.dst, align 1), (load (s512) from @.avm.init, align 1, addrspace 1)

@flash = addrspace(1) constant [64 x i8] zeroinitializer
@ram = global [128 x i8] zeroinitializer
declare void @observe(ptr)

define void @words(ptr %dst)  {
; IR-LABEL: define void @words(
; IR: call{{.*}} void @llvm.memcpy.p0.p1.i16({{.*}}i16 64, i1 false)
; IR-NOT: store 
; IR: ret void
; ASM-LABEL: words:
; ASM: sys memcpy_p
; ASM: ret
; O0-LABEL: words:
  %p0 = getelementptr i8, ptr %dst, i16 0
  store i16 257, ptr %p0, align 1
  %p1 = getelementptr i8, ptr %dst, i16 2
  store i16 2310, ptr %p1, align 1
  %p2 = getelementptr i8, ptr %dst, i16 4
  store i16 4363, ptr %p2, align 1
  %p3 = getelementptr i8, ptr %dst, i16 6
  store i16 6416, ptr %p3, align 1
  %p4 = getelementptr i8, ptr %dst, i16 8
  store i16 8469, ptr %p4, align 1
  %p5 = getelementptr i8, ptr %dst, i16 10
  store i16 10522, ptr %p5, align 1
  %p6 = getelementptr i8, ptr %dst, i16 12
  store i16 12575, ptr %p6, align 1
  %p7 = getelementptr i8, ptr %dst, i16 14
  store i16 14628, ptr %p7, align 1
  %p8 = getelementptr i8, ptr %dst, i16 16
  store i16 16681, ptr %p8, align 1
  %p9 = getelementptr i8, ptr %dst, i16 18
  store i16 18734, ptr %p9, align 1
  %p10 = getelementptr i8, ptr %dst, i16 20
  store i16 20787, ptr %p10, align 1
  %p11 = getelementptr i8, ptr %dst, i16 22
  store i16 22840, ptr %p11, align 1
  %p12 = getelementptr i8, ptr %dst, i16 24
  store i16 24893, ptr %p12, align 1
  %p13 = getelementptr i8, ptr %dst, i16 26
  store i16 26946, ptr %p13, align 1
  %p14 = getelementptr i8, ptr %dst, i16 28
  store i16 28999, ptr %p14, align 1
  %p15 = getelementptr i8, ptr %dst, i16 30
  store i16 31052, ptr %p15, align 1
  %p16 = getelementptr i8, ptr %dst, i16 32
  store i16 33105, ptr %p16, align 1
  %p17 = getelementptr i8, ptr %dst, i16 34
  store i16 35158, ptr %p17, align 1
  %p18 = getelementptr i8, ptr %dst, i16 36
  store i16 37211, ptr %p18, align 1
  %p19 = getelementptr i8, ptr %dst, i16 38
  store i16 39264, ptr %p19, align 1
  %p20 = getelementptr i8, ptr %dst, i16 40
  store i16 41317, ptr %p20, align 1
  %p21 = getelementptr i8, ptr %dst, i16 42
  store i16 43370, ptr %p21, align 1
  %p22 = getelementptr i8, ptr %dst, i16 44
  store i16 45423, ptr %p22, align 1
  %p23 = getelementptr i8, ptr %dst, i16 46
  store i16 47476, ptr %p23, align 1
  %p24 = getelementptr i8, ptr %dst, i16 48
  store i16 49529, ptr %p24, align 1
  %p25 = getelementptr i8, ptr %dst, i16 50
  store i16 51582, ptr %p25, align 1
  %p26 = getelementptr i8, ptr %dst, i16 52
  store i16 53635, ptr %p26, align 1
  %p27 = getelementptr i8, ptr %dst, i16 54
  store i16 55688, ptr %p27, align 1
  %p28 = getelementptr i8, ptr %dst, i16 56
  store i16 57741, ptr %p28, align 1
  %p29 = getelementptr i8, ptr %dst, i16 58
  store i16 59794, ptr %p29, align 1
  %p30 = getelementptr i8, ptr %dst, i16 60
  store i16 61847, ptr %p30, align 1
  %p31 = getelementptr i8, ptr %dst, i16 62
  store i16 63900, ptr %p31, align 1
  ret void
}

define void @descending(ptr %dst)  {
; IR-LABEL: define void @descending(
; IR: call{{.*}} void @llvm.memcpy.p0.p1.i16({{.*}}i16 64, i1 false)
; IR-NOT: store 
; IR: ret void
; ASM-LABEL: descending:
; ASM: sys memcpy_p
; ASM: ret
; O0-LABEL: descending:
  %p0 = getelementptr i8, ptr %dst, i16 62
  store i16 63900, ptr %p0, align 1
  %p1 = getelementptr i8, ptr %dst, i16 60
  store i16 61847, ptr %p1, align 1
  %p2 = getelementptr i8, ptr %dst, i16 58
  store i16 59794, ptr %p2, align 1
  %p3 = getelementptr i8, ptr %dst, i16 56
  store i16 57741, ptr %p3, align 1
  %p4 = getelementptr i8, ptr %dst, i16 54
  store i16 55688, ptr %p4, align 1
  %p5 = getelementptr i8, ptr %dst, i16 52
  store i16 53635, ptr %p5, align 1
  %p6 = getelementptr i8, ptr %dst, i16 50
  store i16 51582, ptr %p6, align 1
  %p7 = getelementptr i8, ptr %dst, i16 48
  store i16 49529, ptr %p7, align 1
  %p8 = getelementptr i8, ptr %dst, i16 46
  store i16 47476, ptr %p8, align 1
  %p9 = getelementptr i8, ptr %dst, i16 44
  store i16 45423, ptr %p9, align 1
  %p10 = getelementptr i8, ptr %dst, i16 42
  store i16 43370, ptr %p10, align 1
  %p11 = getelementptr i8, ptr %dst, i16 40
  store i16 41317, ptr %p11, align 1
  %p12 = getelementptr i8, ptr %dst, i16 38
  store i16 39264, ptr %p12, align 1
  %p13 = getelementptr i8, ptr %dst, i16 36
  store i16 37211, ptr %p13, align 1
  %p14 = getelementptr i8, ptr %dst, i16 34
  store i16 35158, ptr %p14, align 1
  %p15 = getelementptr i8, ptr %dst, i16 32
  store i16 33105, ptr %p15, align 1
  %p16 = getelementptr i8, ptr %dst, i16 30
  store i16 31052, ptr %p16, align 1
  %p17 = getelementptr i8, ptr %dst, i16 28
  store i16 28999, ptr %p17, align 1
  %p18 = getelementptr i8, ptr %dst, i16 26
  store i16 26946, ptr %p18, align 1
  %p19 = getelementptr i8, ptr %dst, i16 24
  store i16 24893, ptr %p19, align 1
  %p20 = getelementptr i8, ptr %dst, i16 22
  store i16 22840, ptr %p20, align 1
  %p21 = getelementptr i8, ptr %dst, i16 20
  store i16 20787, ptr %p21, align 1
  %p22 = getelementptr i8, ptr %dst, i16 18
  store i16 18734, ptr %p22, align 1
  %p23 = getelementptr i8, ptr %dst, i16 16
  store i16 16681, ptr %p23, align 1
  %p24 = getelementptr i8, ptr %dst, i16 14
  store i16 14628, ptr %p24, align 1
  %p25 = getelementptr i8, ptr %dst, i16 12
  store i16 12575, ptr %p25, align 1
  %p26 = getelementptr i8, ptr %dst, i16 10
  store i16 10522, ptr %p26, align 1
  %p27 = getelementptr i8, ptr %dst, i16 8
  store i16 8469, ptr %p27, align 1
  %p28 = getelementptr i8, ptr %dst, i16 6
  store i16 6416, ptr %p28, align 1
  %p29 = getelementptr i8, ptr %dst, i16 4
  store i16 4363, ptr %p29, align 1
  %p30 = getelementptr i8, ptr %dst, i16 2
  store i16 2310, ptr %p30, align 1
  %p31 = getelementptr i8, ptr %dst, i16 0
  store i16 257, ptr %p31, align 1
  ret void
}

define void @negative_offset(ptr %dst)  {
; IR-LABEL: define void @negative_offset(
; IR: call{{.*}} void @llvm.memcpy.p0.p1.i16({{.*}}i16 64, i1 false)
; IR-NOT: store 
; IR: ret void
; ASM-LABEL: negative_offset:
; ASM: sys memcpy_p
; ASM: ret
; O0-LABEL: negative_offset:
  %p0 = getelementptr i8, ptr %dst, i16 -16
  store i16 257, ptr %p0, align 1
  %p1 = getelementptr i8, ptr %dst, i16 -14
  store i16 2310, ptr %p1, align 1
  %p2 = getelementptr i8, ptr %dst, i16 -12
  store i16 4363, ptr %p2, align 1
  %p3 = getelementptr i8, ptr %dst, i16 -10
  store i16 6416, ptr %p3, align 1
  %p4 = getelementptr i8, ptr %dst, i16 -8
  store i16 8469, ptr %p4, align 1
  %p5 = getelementptr i8, ptr %dst, i16 -6
  store i16 10522, ptr %p5, align 1
  %p6 = getelementptr i8, ptr %dst, i16 -4
  store i16 12575, ptr %p6, align 1
  %p7 = getelementptr i8, ptr %dst, i16 -2
  store i16 14628, ptr %p7, align 1
  %p8 = getelementptr i8, ptr %dst, i16 0
  store i16 16681, ptr %p8, align 1
  %p9 = getelementptr i8, ptr %dst, i16 2
  store i16 18734, ptr %p9, align 1
  %p10 = getelementptr i8, ptr %dst, i16 4
  store i16 20787, ptr %p10, align 1
  %p11 = getelementptr i8, ptr %dst, i16 6
  store i16 22840, ptr %p11, align 1
  %p12 = getelementptr i8, ptr %dst, i16 8
  store i16 24893, ptr %p12, align 1
  %p13 = getelementptr i8, ptr %dst, i16 10
  store i16 26946, ptr %p13, align 1
  %p14 = getelementptr i8, ptr %dst, i16 12
  store i16 28999, ptr %p14, align 1
  %p15 = getelementptr i8, ptr %dst, i16 14
  store i16 31052, ptr %p15, align 1
  %p16 = getelementptr i8, ptr %dst, i16 16
  store i16 33105, ptr %p16, align 1
  %p17 = getelementptr i8, ptr %dst, i16 18
  store i16 35158, ptr %p17, align 1
  %p18 = getelementptr i8, ptr %dst, i16 20
  store i16 37211, ptr %p18, align 1
  %p19 = getelementptr i8, ptr %dst, i16 22
  store i16 39264, ptr %p19, align 1
  %p20 = getelementptr i8, ptr %dst, i16 24
  store i16 41317, ptr %p20, align 1
  %p21 = getelementptr i8, ptr %dst, i16 26
  store i16 43370, ptr %p21, align 1
  %p22 = getelementptr i8, ptr %dst, i16 28
  store i16 45423, ptr %p22, align 1
  %p23 = getelementptr i8, ptr %dst, i16 30
  store i16 47476, ptr %p23, align 1
  %p24 = getelementptr i8, ptr %dst, i16 32
  store i16 49529, ptr %p24, align 1
  %p25 = getelementptr i8, ptr %dst, i16 34
  store i16 51582, ptr %p25, align 1
  %p26 = getelementptr i8, ptr %dst, i16 36
  store i16 53635, ptr %p26, align 1
  %p27 = getelementptr i8, ptr %dst, i16 38
  store i16 55688, ptr %p27, align 1
  %p28 = getelementptr i8, ptr %dst, i16 40
  store i16 57741, ptr %p28, align 1
  %p29 = getelementptr i8, ptr %dst, i16 42
  store i16 59794, ptr %p29, align 1
  %p30 = getelementptr i8, ptr %dst, i16 44
  store i16 61847, ptr %p30, align 1
  %p31 = getelementptr i8, ptr %dst, i16 46
  store i16 63900, ptr %p31, align 1
  ret void
}

define void @bytes(ptr %dst)  {
; IR-LABEL: define void @bytes(
; IR: call{{.*}} void @llvm.memcpy.p0.p1.i16({{.*}}i16 32, i1 false)
; IR-NOT: store 
; IR: ret void
; ASM-LABEL: bytes:
; ASM: sys memcpy_p
; ASM: ret
; O0-LABEL: bytes:
  %p0 = getelementptr i8, ptr %dst, i16 0
  store i8 17, ptr %p0, align 1
  %p1 = getelementptr i8, ptr %dst, i16 1
  store i8 20, ptr %p1, align 1
  %p2 = getelementptr i8, ptr %dst, i16 2
  store i8 23, ptr %p2, align 1
  %p3 = getelementptr i8, ptr %dst, i16 3
  store i8 26, ptr %p3, align 1
  %p4 = getelementptr i8, ptr %dst, i16 4
  store i8 29, ptr %p4, align 1
  %p5 = getelementptr i8, ptr %dst, i16 5
  store i8 32, ptr %p5, align 1
  %p6 = getelementptr i8, ptr %dst, i16 6
  store i8 35, ptr %p6, align 1
  %p7 = getelementptr i8, ptr %dst, i16 7
  store i8 38, ptr %p7, align 1
  %p8 = getelementptr i8, ptr %dst, i16 8
  store i8 41, ptr %p8, align 1
  %p9 = getelementptr i8, ptr %dst, i16 9
  store i8 44, ptr %p9, align 1
  %p10 = getelementptr i8, ptr %dst, i16 10
  store i8 47, ptr %p10, align 1
  %p11 = getelementptr i8, ptr %dst, i16 11
  store i8 50, ptr %p11, align 1
  %p12 = getelementptr i8, ptr %dst, i16 12
  store i8 53, ptr %p12, align 1
  %p13 = getelementptr i8, ptr %dst, i16 13
  store i8 56, ptr %p13, align 1
  %p14 = getelementptr i8, ptr %dst, i16 14
  store i8 59, ptr %p14, align 1
  %p15 = getelementptr i8, ptr %dst, i16 15
  store i8 62, ptr %p15, align 1
  %p16 = getelementptr i8, ptr %dst, i16 16
  store i8 65, ptr %p16, align 1
  %p17 = getelementptr i8, ptr %dst, i16 17
  store i8 68, ptr %p17, align 1
  %p18 = getelementptr i8, ptr %dst, i16 18
  store i8 71, ptr %p18, align 1
  %p19 = getelementptr i8, ptr %dst, i16 19
  store i8 74, ptr %p19, align 1
  %p20 = getelementptr i8, ptr %dst, i16 20
  store i8 77, ptr %p20, align 1
  %p21 = getelementptr i8, ptr %dst, i16 21
  store i8 80, ptr %p21, align 1
  %p22 = getelementptr i8, ptr %dst, i16 22
  store i8 83, ptr %p22, align 1
  %p23 = getelementptr i8, ptr %dst, i16 23
  store i8 86, ptr %p23, align 1
  %p24 = getelementptr i8, ptr %dst, i16 24
  store i8 89, ptr %p24, align 1
  %p25 = getelementptr i8, ptr %dst, i16 25
  store i8 92, ptr %p25, align 1
  %p26 = getelementptr i8, ptr %dst, i16 26
  store i8 95, ptr %p26, align 1
  %p27 = getelementptr i8, ptr %dst, i16 27
  store i8 98, ptr %p27, align 1
  %p28 = getelementptr i8, ptr %dst, i16 28
  store i8 101, ptr %p28, align 1
  %p29 = getelementptr i8, ptr %dst, i16 29
  store i8 104, ptr %p29, align 1
  %p30 = getelementptr i8, ptr %dst, i16 30
  store i8 107, ptr %p30, align 1
  %p31 = getelementptr i8, ptr %dst, i16 31
  store i8 110, ptr %p31, align 1
  ret void
}

define void @wide(ptr %dst)  {
; IR-LABEL: define void @wide(
; IR: call{{.*}} void @llvm.memcpy.p0.p1.i16({{.*}}i16 64, i1 false)
; IR-NOT: store 
; IR: ret void
; ASM-LABEL: wide:
; ASM: sys memcpy_p
; ASM: ret
; O0-LABEL: wide:
  %p0 = getelementptr i8, ptr %dst, i16 0
  store i32 16909060, ptr %p0, align 1
  %p1 = getelementptr i8, ptr %dst, i16 4
  store i32 35997803, ptr %p1, align 1
  %p2 = getelementptr i8, ptr %dst, i16 8
  store i32 55086546, ptr %p2, align 1
  %p3 = getelementptr i8, ptr %dst, i16 12
  store i32 74175289, ptr %p3, align 1
  %p4 = getelementptr i8, ptr %dst, i16 16
  store i32 93264032, ptr %p4, align 1
  %p5 = getelementptr i8, ptr %dst, i16 20
  store i32 112352775, ptr %p5, align 1
  %p6 = getelementptr i8, ptr %dst, i16 24
  store i32 131441518, ptr %p6, align 1
  %p7 = getelementptr i8, ptr %dst, i16 28
  store i32 150530261, ptr %p7, align 1
  %p8 = getelementptr i8, ptr %dst, i16 32
  store i32 169619004, ptr %p8, align 1
  %p9 = getelementptr i8, ptr %dst, i16 36
  store i32 188707747, ptr %p9, align 1
  %p10 = getelementptr i8, ptr %dst, i16 40
  store i32 207796490, ptr %p10, align 1
  %p11 = getelementptr i8, ptr %dst, i16 44
  store i32 226885233, ptr %p11, align 1
  %p12 = getelementptr i8, ptr %dst, i16 48
  store i32 245973976, ptr %p12, align 1
  %p13 = getelementptr i8, ptr %dst, i16 52
  store i32 265062719, ptr %p13, align 1
  %p14 = getelementptr i8, ptr %dst, i16 56
  store i32 284151462, ptr %p14, align 1
  %p15 = getelementptr i8, ptr %dst, i16 60
  store i32 303240205, ptr %p15, align 1
  ret void
}

define void @mixed_record_fields(ptr %dst)  {
; IR-LABEL: define void @mixed_record_fields(
; IR: call{{.*}} void @llvm.memcpy.p0.p1.i16({{.*}}i16 56, i1 false)
; IR-NOT: store 
; IR: ret void
; ASM-LABEL: mixed_record_fields:
; ASM: sys memcpy_p
; ASM: ret
; O0-LABEL: mixed_record_fields:
  %p0 = getelementptr i8, ptr %dst, i16 0
  store i8 11, ptr %p0, align 1
  %p1 = getelementptr i8, ptr %dst, i16 1
  store i16 257, ptr %p1, align 1
  %p2 = getelementptr i8, ptr %dst, i16 3
  store float 1.000000e+00, ptr %p2, align 1
  %p3 = getelementptr i8, ptr %dst, i16 7
  store i8 12, ptr %p3, align 1
  %p4 = getelementptr i8, ptr %dst, i16 8
  store i16 4356, ptr %p4, align 1
  %p5 = getelementptr i8, ptr %dst, i16 10
  store float 2.000000e+00, ptr %p5, align 1
  %p6 = getelementptr i8, ptr %dst, i16 14
  store i8 13, ptr %p6, align 1
  %p7 = getelementptr i8, ptr %dst, i16 15
  store i16 8455, ptr %p7, align 1
  %p8 = getelementptr i8, ptr %dst, i16 17
  store float 3.000000e+00, ptr %p8, align 1
  %p9 = getelementptr i8, ptr %dst, i16 21
  store i8 14, ptr %p9, align 1
  %p10 = getelementptr i8, ptr %dst, i16 22
  store i16 12554, ptr %p10, align 1
  %p11 = getelementptr i8, ptr %dst, i16 24
  store float 4.000000e+00, ptr %p11, align 1
  %p12 = getelementptr i8, ptr %dst, i16 28
  store i8 15, ptr %p12, align 1
  %p13 = getelementptr i8, ptr %dst, i16 29
  store i16 16653, ptr %p13, align 1
  %p14 = getelementptr i8, ptr %dst, i16 31
  store float 5.000000e+00, ptr %p14, align 1
  %p15 = getelementptr i8, ptr %dst, i16 35
  store i8 16, ptr %p15, align 1
  %p16 = getelementptr i8, ptr %dst, i16 36
  store i16 20752, ptr %p16, align 1
  %p17 = getelementptr i8, ptr %dst, i16 38
  store float 6.000000e+00, ptr %p17, align 1
  %p18 = getelementptr i8, ptr %dst, i16 42
  store i8 17, ptr %p18, align 1
  %p19 = getelementptr i8, ptr %dst, i16 43
  store i16 24851, ptr %p19, align 1
  %p20 = getelementptr i8, ptr %dst, i16 45
  store float 7.000000e+00, ptr %p20, align 1
  %p21 = getelementptr i8, ptr %dst, i16 49
  store i8 18, ptr %p21, align 1
  %p22 = getelementptr i8, ptr %dst, i16 50
  store i16 28950, ptr %p22, align 1
  %p23 = getelementptr i8, ptr %dst, i16 52
  store float 8.000000e+00, ptr %p23, align 1
  ret void
}

define void @aggregate(ptr %dst)  {
; IR-LABEL: define void @aggregate(
; IR: call{{.*}} void @llvm.memcpy.p0.p1.i16({{.*}}i16 32, i1 false)
; IR-NOT: store 
; IR: ret void
; ASM-LABEL: aggregate:
; ASM: sys memcpy_p
; ASM: ret
; O0-LABEL: aggregate:
  %p0 = getelementptr i8, ptr %dst, i16 0
  store [16 x i16] [i16 257, i16 2310, i16 4363, i16 6416, i16 8469, i16 10522, i16 12575, i16 14628, i16 16681, i16 18734, i16 20787, i16 22840, i16 24893, i16 26946, i16 28999, i16 31052], ptr %p0, align 1
  ret void
}

define void @program_pointer_layout(ptr %dst)  {
; IR-LABEL: define void @program_pointer_layout(
; IR: call{{.*}} void @llvm.memcpy.p0.p1.i16({{.*}}i16 60, i1 false)
; IR-NOT: store 
; IR: ret void
; ASM-LABEL: program_pointer_layout:
; ASM: sys memcpy_p
; ASM: ret
; O0-LABEL: program_pointer_layout:
  %p0 = getelementptr i8, ptr %dst, i16 0
  store ptr addrspace(1) getelementptr (i8, ptr addrspace(1) @flash, i24 0), ptr %p0, align 1
  %p1 = getelementptr i8, ptr %dst, i16 3
  store ptr addrspace(1) getelementptr (i8, ptr addrspace(1) @flash, i24 1), ptr %p1, align 1
  %p2 = getelementptr i8, ptr %dst, i16 6
  store ptr addrspace(1) getelementptr (i8, ptr addrspace(1) @flash, i24 2), ptr %p2, align 1
  %p3 = getelementptr i8, ptr %dst, i16 9
  store ptr addrspace(1) getelementptr (i8, ptr addrspace(1) @flash, i24 3), ptr %p3, align 1
  %p4 = getelementptr i8, ptr %dst, i16 12
  store ptr addrspace(1) getelementptr (i8, ptr addrspace(1) @flash, i24 4), ptr %p4, align 1
  %p5 = getelementptr i8, ptr %dst, i16 15
  store ptr addrspace(1) getelementptr (i8, ptr addrspace(1) @flash, i24 5), ptr %p5, align 1
  %p6 = getelementptr i8, ptr %dst, i16 18
  store ptr addrspace(1) getelementptr (i8, ptr addrspace(1) @flash, i24 6), ptr %p6, align 1
  %p7 = getelementptr i8, ptr %dst, i16 21
  store ptr addrspace(1) getelementptr (i8, ptr addrspace(1) @flash, i24 7), ptr %p7, align 1
  %p8 = getelementptr i8, ptr %dst, i16 24
  store ptr addrspace(1) getelementptr (i8, ptr addrspace(1) @flash, i24 8), ptr %p8, align 1
  %p9 = getelementptr i8, ptr %dst, i16 27
  store ptr addrspace(1) getelementptr (i8, ptr addrspace(1) @flash, i24 9), ptr %p9, align 1
  %p10 = getelementptr i8, ptr %dst, i16 30
  store ptr addrspace(1) getelementptr (i8, ptr addrspace(1) @flash, i24 10), ptr %p10, align 1
  %p11 = getelementptr i8, ptr %dst, i16 33
  store ptr addrspace(1) getelementptr (i8, ptr addrspace(1) @flash, i24 11), ptr %p11, align 1
  %p12 = getelementptr i8, ptr %dst, i16 36
  store ptr addrspace(1) getelementptr (i8, ptr addrspace(1) @flash, i24 12), ptr %p12, align 1
  %p13 = getelementptr i8, ptr %dst, i16 39
  store ptr addrspace(1) getelementptr (i8, ptr addrspace(1) @flash, i24 13), ptr %p13, align 1
  %p14 = getelementptr i8, ptr %dst, i16 42
  store ptr addrspace(1) getelementptr (i8, ptr addrspace(1) @flash, i24 14), ptr %p14, align 1
  %p15 = getelementptr i8, ptr %dst, i16 45
  store ptr addrspace(1) getelementptr (i8, ptr addrspace(1) @flash, i24 15), ptr %p15, align 1
  %p16 = getelementptr i8, ptr %dst, i16 48
  store ptr addrspace(1) getelementptr (i8, ptr addrspace(1) @flash, i24 16), ptr %p16, align 1
  %p17 = getelementptr i8, ptr %dst, i16 51
  store ptr addrspace(1) getelementptr (i8, ptr addrspace(1) @flash, i24 17), ptr %p17, align 1
  %p18 = getelementptr i8, ptr %dst, i16 54
  store ptr addrspace(1) getelementptr (i8, ptr addrspace(1) @flash, i24 18), ptr %p18, align 1
  %p19 = getelementptr i8, ptr %dst, i16 57
  store ptr addrspace(1) getelementptr (i8, ptr addrspace(1) @flash, i24 19), ptr %p19, align 1
  ret void
}

define void @uniform(ptr %dst)  {
; IR-LABEL: define void @uniform(
; IR: call{{.*}} void @llvm.memset.p0.i16({{.*}}i16 64, i1 false)
; IR-NOT: store 
; IR: ret void
; ASM-LABEL: uniform:
; ASM: sys memset
; ASM: ret
; O0-LABEL: uniform:
  %p0 = getelementptr i8, ptr %dst, i16 0
  store i8 90, ptr %p0, align 1
  %p1 = getelementptr i8, ptr %dst, i16 1
  store i8 90, ptr %p1, align 1
  %p2 = getelementptr i8, ptr %dst, i16 2
  store i8 90, ptr %p2, align 1
  %p3 = getelementptr i8, ptr %dst, i16 3
  store i8 90, ptr %p3, align 1
  %p4 = getelementptr i8, ptr %dst, i16 4
  store i8 90, ptr %p4, align 1
  %p5 = getelementptr i8, ptr %dst, i16 5
  store i8 90, ptr %p5, align 1
  %p6 = getelementptr i8, ptr %dst, i16 6
  store i8 90, ptr %p6, align 1
  %p7 = getelementptr i8, ptr %dst, i16 7
  store i8 90, ptr %p7, align 1
  %p8 = getelementptr i8, ptr %dst, i16 8
  store i8 90, ptr %p8, align 1
  %p9 = getelementptr i8, ptr %dst, i16 9
  store i8 90, ptr %p9, align 1
  %p10 = getelementptr i8, ptr %dst, i16 10
  store i8 90, ptr %p10, align 1
  %p11 = getelementptr i8, ptr %dst, i16 11
  store i8 90, ptr %p11, align 1
  %p12 = getelementptr i8, ptr %dst, i16 12
  store i8 90, ptr %p12, align 1
  %p13 = getelementptr i8, ptr %dst, i16 13
  store i8 90, ptr %p13, align 1
  %p14 = getelementptr i8, ptr %dst, i16 14
  store i8 90, ptr %p14, align 1
  %p15 = getelementptr i8, ptr %dst, i16 15
  store i8 90, ptr %p15, align 1
  %p16 = getelementptr i8, ptr %dst, i16 16
  store i8 90, ptr %p16, align 1
  %p17 = getelementptr i8, ptr %dst, i16 17
  store i8 90, ptr %p17, align 1
  %p18 = getelementptr i8, ptr %dst, i16 18
  store i8 90, ptr %p18, align 1
  %p19 = getelementptr i8, ptr %dst, i16 19
  store i8 90, ptr %p19, align 1
  %p20 = getelementptr i8, ptr %dst, i16 20
  store i8 90, ptr %p20, align 1
  %p21 = getelementptr i8, ptr %dst, i16 21
  store i8 90, ptr %p21, align 1
  %p22 = getelementptr i8, ptr %dst, i16 22
  store i8 90, ptr %p22, align 1
  %p23 = getelementptr i8, ptr %dst, i16 23
  store i8 90, ptr %p23, align 1
  %p24 = getelementptr i8, ptr %dst, i16 24
  store i8 90, ptr %p24, align 1
  %p25 = getelementptr i8, ptr %dst, i16 25
  store i8 90, ptr %p25, align 1
  %p26 = getelementptr i8, ptr %dst, i16 26
  store i8 90, ptr %p26, align 1
  %p27 = getelementptr i8, ptr %dst, i16 27
  store i8 90, ptr %p27, align 1
  %p28 = getelementptr i8, ptr %dst, i16 28
  store i8 90, ptr %p28, align 1
  %p29 = getelementptr i8, ptr %dst, i16 29
  store i8 90, ptr %p29, align 1
  %p30 = getelementptr i8, ptr %dst, i16 30
  store i8 90, ptr %p30, align 1
  %p31 = getelementptr i8, ptr %dst, i16 31
  store i8 90, ptr %p31, align 1
  %p32 = getelementptr i8, ptr %dst, i16 32
  store i8 90, ptr %p32, align 1
  %p33 = getelementptr i8, ptr %dst, i16 33
  store i8 90, ptr %p33, align 1
  %p34 = getelementptr i8, ptr %dst, i16 34
  store i8 90, ptr %p34, align 1
  %p35 = getelementptr i8, ptr %dst, i16 35
  store i8 90, ptr %p35, align 1
  %p36 = getelementptr i8, ptr %dst, i16 36
  store i8 90, ptr %p36, align 1
  %p37 = getelementptr i8, ptr %dst, i16 37
  store i8 90, ptr %p37, align 1
  %p38 = getelementptr i8, ptr %dst, i16 38
  store i8 90, ptr %p38, align 1
  %p39 = getelementptr i8, ptr %dst, i16 39
  store i8 90, ptr %p39, align 1
  %p40 = getelementptr i8, ptr %dst, i16 40
  store i8 90, ptr %p40, align 1
  %p41 = getelementptr i8, ptr %dst, i16 41
  store i8 90, ptr %p41, align 1
  %p42 = getelementptr i8, ptr %dst, i16 42
  store i8 90, ptr %p42, align 1
  %p43 = getelementptr i8, ptr %dst, i16 43
  store i8 90, ptr %p43, align 1
  %p44 = getelementptr i8, ptr %dst, i16 44
  store i8 90, ptr %p44, align 1
  %p45 = getelementptr i8, ptr %dst, i16 45
  store i8 90, ptr %p45, align 1
  %p46 = getelementptr i8, ptr %dst, i16 46
  store i8 90, ptr %p46, align 1
  %p47 = getelementptr i8, ptr %dst, i16 47
  store i8 90, ptr %p47, align 1
  %p48 = getelementptr i8, ptr %dst, i16 48
  store i8 90, ptr %p48, align 1
  %p49 = getelementptr i8, ptr %dst, i16 49
  store i8 90, ptr %p49, align 1
  %p50 = getelementptr i8, ptr %dst, i16 50
  store i8 90, ptr %p50, align 1
  %p51 = getelementptr i8, ptr %dst, i16 51
  store i8 90, ptr %p51, align 1
  %p52 = getelementptr i8, ptr %dst, i16 52
  store i8 90, ptr %p52, align 1
  %p53 = getelementptr i8, ptr %dst, i16 53
  store i8 90, ptr %p53, align 1
  %p54 = getelementptr i8, ptr %dst, i16 54
  store i8 90, ptr %p54, align 1
  %p55 = getelementptr i8, ptr %dst, i16 55
  store i8 90, ptr %p55, align 1
  %p56 = getelementptr i8, ptr %dst, i16 56
  store i8 90, ptr %p56, align 1
  %p57 = getelementptr i8, ptr %dst, i16 57
  store i8 90, ptr %p57, align 1
  %p58 = getelementptr i8, ptr %dst, i16 58
  store i8 90, ptr %p58, align 1
  %p59 = getelementptr i8, ptr %dst, i16 59
  store i8 90, ptr %p59, align 1
  %p60 = getelementptr i8, ptr %dst, i16 60
  store i8 90, ptr %p60, align 1
  %p61 = getelementptr i8, ptr %dst, i16 61
  store i8 90, ptr %p61, align 1
  %p62 = getelementptr i8, ptr %dst, i16 62
  store i8 90, ptr %p62, align 1
  %p63 = getelementptr i8, ptr %dst, i16 63
  store i8 90, ptr %p63, align 1
  ret void
}

define void @small(ptr %dst)  {
; IR-LABEL: define void @small(
; IR-NOT: call void @llvm.memcpy
; IR-NOT: call void @llvm.memset
; IR: store 
; IR: ret void
; ASM-LABEL: small:
; ASM-NOT: sys memcpy_p
; ASM: ret
; O0-LABEL: small:
  %p0 = getelementptr i8, ptr %dst, i16 0
  store i16 257, ptr %p0, align 1
  %p1 = getelementptr i8, ptr %dst, i16 2
  store i16 2310, ptr %p1, align 1
  %p2 = getelementptr i8, ptr %dst, i16 4
  store i16 4363, ptr %p2, align 1
  %p3 = getelementptr i8, ptr %dst, i16 6
  store i16 6416, ptr %p3, align 1
  ret void
}

define void @cheap_repeated_words(ptr %dst)  {
; IR-LABEL: define void @cheap_repeated_words(
; IR-NOT: call void @llvm.memcpy
; IR-NOT: call void @llvm.memset
; IR: store 
; IR: ret void
; ASM-LABEL: cheap_repeated_words:
; ASM-NOT: sys memcpy_p
; ASM: ret
; O0-LABEL: cheap_repeated_words:
  %p0 = getelementptr i8, ptr %dst, i16 0
  store i16 4660, ptr %p0, align 1
  %p1 = getelementptr i8, ptr %dst, i16 2
  store i16 4660, ptr %p1, align 1
  %p2 = getelementptr i8, ptr %dst, i16 4
  store i16 4660, ptr %p2, align 1
  %p3 = getelementptr i8, ptr %dst, i16 6
  store i16 4660, ptr %p3, align 1
  %p4 = getelementptr i8, ptr %dst, i16 8
  store i16 4660, ptr %p4, align 1
  %p5 = getelementptr i8, ptr %dst, i16 10
  store i16 4660, ptr %p5, align 1
  %p6 = getelementptr i8, ptr %dst, i16 12
  store i16 4660, ptr %p6, align 1
  %p7 = getelementptr i8, ptr %dst, i16 14
  store i16 4660, ptr %p7, align 1
  %p8 = getelementptr i8, ptr %dst, i16 16
  store i16 4660, ptr %p8, align 1
  %p9 = getelementptr i8, ptr %dst, i16 18
  store i16 4660, ptr %p9, align 1
  %p10 = getelementptr i8, ptr %dst, i16 20
  store i16 4660, ptr %p10, align 1
  %p11 = getelementptr i8, ptr %dst, i16 22
  store i16 4660, ptr %p11, align 1
  %p12 = getelementptr i8, ptr %dst, i16 24
  store i16 4660, ptr %p12, align 1
  %p13 = getelementptr i8, ptr %dst, i16 26
  store i16 4660, ptr %p13, align 1
  %p14 = getelementptr i8, ptr %dst, i16 28
  store i16 4660, ptr %p14, align 1
  %p15 = getelementptr i8, ptr %dst, i16 30
  store i16 4660, ptr %p15, align 1
  %p16 = getelementptr i8, ptr %dst, i16 32
  store i16 4660, ptr %p16, align 1
  %p17 = getelementptr i8, ptr %dst, i16 34
  store i16 4660, ptr %p17, align 1
  %p18 = getelementptr i8, ptr %dst, i16 36
  store i16 4660, ptr %p18, align 1
  %p19 = getelementptr i8, ptr %dst, i16 38
  store i16 4660, ptr %p19, align 1
  %p20 = getelementptr i8, ptr %dst, i16 40
  store i16 4660, ptr %p20, align 1
  %p21 = getelementptr i8, ptr %dst, i16 42
  store i16 4660, ptr %p21, align 1
  %p22 = getelementptr i8, ptr %dst, i16 44
  store i16 4660, ptr %p22, align 1
  %p23 = getelementptr i8, ptr %dst, i16 46
  store i16 4660, ptr %p23, align 1
  %p24 = getelementptr i8, ptr %dst, i16 48
  store i16 4660, ptr %p24, align 1
  %p25 = getelementptr i8, ptr %dst, i16 50
  store i16 4660, ptr %p25, align 1
  %p26 = getelementptr i8, ptr %dst, i16 52
  store i16 4660, ptr %p26, align 1
  %p27 = getelementptr i8, ptr %dst, i16 54
  store i16 4660, ptr %p27, align 1
  %p28 = getelementptr i8, ptr %dst, i16 56
  store i16 4660, ptr %p28, align 1
  %p29 = getelementptr i8, ptr %dst, i16 58
  store i16 4660, ptr %p29, align 1
  %p30 = getelementptr i8, ptr %dst, i16 60
  store i16 4660, ptr %p30, align 1
  %p31 = getelementptr i8, ptr %dst, i16 62
  store i16 4660, ptr %p31, align 1
  ret void
}

define void @volatile(ptr %dst)  {
; IR-LABEL: define void @volatile(
; IR-NOT: call void @llvm.memcpy
; IR-NOT: call void @llvm.memset
; IR: store 
; IR: ret void
; ASM-LABEL: volatile:
; ASM-NOT: sys memcpy_p
; ASM: ret
; O0-LABEL: volatile:
  %p0 = getelementptr i8, ptr %dst, i16 0
  store volatile i16 257, ptr %p0, align 1
  %p1 = getelementptr i8, ptr %dst, i16 2
  store volatile i16 2310, ptr %p1, align 1
  %p2 = getelementptr i8, ptr %dst, i16 4
  store volatile i16 4363, ptr %p2, align 1
  %p3 = getelementptr i8, ptr %dst, i16 6
  store volatile i16 6416, ptr %p3, align 1
  %p4 = getelementptr i8, ptr %dst, i16 8
  store volatile i16 8469, ptr %p4, align 1
  %p5 = getelementptr i8, ptr %dst, i16 10
  store volatile i16 10522, ptr %p5, align 1
  %p6 = getelementptr i8, ptr %dst, i16 12
  store volatile i16 12575, ptr %p6, align 1
  %p7 = getelementptr i8, ptr %dst, i16 14
  store volatile i16 14628, ptr %p7, align 1
  %p8 = getelementptr i8, ptr %dst, i16 16
  store volatile i16 16681, ptr %p8, align 1
  %p9 = getelementptr i8, ptr %dst, i16 18
  store volatile i16 18734, ptr %p9, align 1
  %p10 = getelementptr i8, ptr %dst, i16 20
  store volatile i16 20787, ptr %p10, align 1
  %p11 = getelementptr i8, ptr %dst, i16 22
  store volatile i16 22840, ptr %p11, align 1
  %p12 = getelementptr i8, ptr %dst, i16 24
  store volatile i16 24893, ptr %p12, align 1
  %p13 = getelementptr i8, ptr %dst, i16 26
  store volatile i16 26946, ptr %p13, align 1
  %p14 = getelementptr i8, ptr %dst, i16 28
  store volatile i16 28999, ptr %p14, align 1
  %p15 = getelementptr i8, ptr %dst, i16 30
  store volatile i16 31052, ptr %p15, align 1
  %p16 = getelementptr i8, ptr %dst, i16 32
  store volatile i16 33105, ptr %p16, align 1
  %p17 = getelementptr i8, ptr %dst, i16 34
  store volatile i16 35158, ptr %p17, align 1
  %p18 = getelementptr i8, ptr %dst, i16 36
  store volatile i16 37211, ptr %p18, align 1
  %p19 = getelementptr i8, ptr %dst, i16 38
  store volatile i16 39264, ptr %p19, align 1
  %p20 = getelementptr i8, ptr %dst, i16 40
  store volatile i16 41317, ptr %p20, align 1
  %p21 = getelementptr i8, ptr %dst, i16 42
  store volatile i16 43370, ptr %p21, align 1
  %p22 = getelementptr i8, ptr %dst, i16 44
  store volatile i16 45423, ptr %p22, align 1
  %p23 = getelementptr i8, ptr %dst, i16 46
  store volatile i16 47476, ptr %p23, align 1
  %p24 = getelementptr i8, ptr %dst, i16 48
  store volatile i16 49529, ptr %p24, align 1
  %p25 = getelementptr i8, ptr %dst, i16 50
  store volatile i16 51582, ptr %p25, align 1
  %p26 = getelementptr i8, ptr %dst, i16 52
  store volatile i16 53635, ptr %p26, align 1
  %p27 = getelementptr i8, ptr %dst, i16 54
  store volatile i16 55688, ptr %p27, align 1
  %p28 = getelementptr i8, ptr %dst, i16 56
  store volatile i16 57741, ptr %p28, align 1
  %p29 = getelementptr i8, ptr %dst, i16 58
  store volatile i16 59794, ptr %p29, align 1
  %p30 = getelementptr i8, ptr %dst, i16 60
  store volatile i16 61847, ptr %p30, align 1
  %p31 = getelementptr i8, ptr %dst, i16 62
  store volatile i16 63900, ptr %p31, align 1
  ret void
}

define void @atomic(ptr %dst)  {
; IR-LABEL: define void @atomic(
; IR-NOT: call void @llvm.memcpy
; IR-NOT: call void @llvm.memset
; IR: store 
; IR: ret void
; ASM-LABEL: atomic:
; ASM-NOT: sys memcpy_p
; ASM: ret
; O0-LABEL: atomic:
  %p0 = getelementptr i8, ptr %dst, i16 0
  store atomic i16 257, ptr %p0 unordered, align 1
  %p1 = getelementptr i8, ptr %dst, i16 2
  store atomic i16 2310, ptr %p1 unordered, align 1
  %p2 = getelementptr i8, ptr %dst, i16 4
  store atomic i16 4363, ptr %p2 unordered, align 1
  %p3 = getelementptr i8, ptr %dst, i16 6
  store atomic i16 6416, ptr %p3 unordered, align 1
  %p4 = getelementptr i8, ptr %dst, i16 8
  store atomic i16 8469, ptr %p4 unordered, align 1
  %p5 = getelementptr i8, ptr %dst, i16 10
  store atomic i16 10522, ptr %p5 unordered, align 1
  %p6 = getelementptr i8, ptr %dst, i16 12
  store atomic i16 12575, ptr %p6 unordered, align 1
  %p7 = getelementptr i8, ptr %dst, i16 14
  store atomic i16 14628, ptr %p7 unordered, align 1
  %p8 = getelementptr i8, ptr %dst, i16 16
  store atomic i16 16681, ptr %p8 unordered, align 1
  %p9 = getelementptr i8, ptr %dst, i16 18
  store atomic i16 18734, ptr %p9 unordered, align 1
  %p10 = getelementptr i8, ptr %dst, i16 20
  store atomic i16 20787, ptr %p10 unordered, align 1
  %p11 = getelementptr i8, ptr %dst, i16 22
  store atomic i16 22840, ptr %p11 unordered, align 1
  %p12 = getelementptr i8, ptr %dst, i16 24
  store atomic i16 24893, ptr %p12 unordered, align 1
  %p13 = getelementptr i8, ptr %dst, i16 26
  store atomic i16 26946, ptr %p13 unordered, align 1
  %p14 = getelementptr i8, ptr %dst, i16 28
  store atomic i16 28999, ptr %p14 unordered, align 1
  %p15 = getelementptr i8, ptr %dst, i16 30
  store atomic i16 31052, ptr %p15 unordered, align 1
  %p16 = getelementptr i8, ptr %dst, i16 32
  store atomic i16 33105, ptr %p16 unordered, align 1
  %p17 = getelementptr i8, ptr %dst, i16 34
  store atomic i16 35158, ptr %p17 unordered, align 1
  %p18 = getelementptr i8, ptr %dst, i16 36
  store atomic i16 37211, ptr %p18 unordered, align 1
  %p19 = getelementptr i8, ptr %dst, i16 38
  store atomic i16 39264, ptr %p19 unordered, align 1
  %p20 = getelementptr i8, ptr %dst, i16 40
  store atomic i16 41317, ptr %p20 unordered, align 1
  %p21 = getelementptr i8, ptr %dst, i16 42
  store atomic i16 43370, ptr %p21 unordered, align 1
  %p22 = getelementptr i8, ptr %dst, i16 44
  store atomic i16 45423, ptr %p22 unordered, align 1
  %p23 = getelementptr i8, ptr %dst, i16 46
  store atomic i16 47476, ptr %p23 unordered, align 1
  %p24 = getelementptr i8, ptr %dst, i16 48
  store atomic i16 49529, ptr %p24 unordered, align 1
  %p25 = getelementptr i8, ptr %dst, i16 50
  store atomic i16 51582, ptr %p25 unordered, align 1
  %p26 = getelementptr i8, ptr %dst, i16 52
  store atomic i16 53635, ptr %p26 unordered, align 1
  %p27 = getelementptr i8, ptr %dst, i16 54
  store atomic i16 55688, ptr %p27 unordered, align 1
  %p28 = getelementptr i8, ptr %dst, i16 56
  store atomic i16 57741, ptr %p28 unordered, align 1
  %p29 = getelementptr i8, ptr %dst, i16 58
  store atomic i16 59794, ptr %p29 unordered, align 1
  %p30 = getelementptr i8, ptr %dst, i16 60
  store atomic i16 61847, ptr %p30 unordered, align 1
  %p31 = getelementptr i8, ptr %dst, i16 62
  store atomic i16 63900, ptr %p31 unordered, align 1
  ret void
}

define void @call_barrier(ptr %dst)  {
; IR-LABEL: define void @call_barrier(
; IR-NOT: call void @llvm.memcpy
; IR-NOT: call void @llvm.memset
; IR: store 
; IR: ret void
; ASM-LABEL: call_barrier:
; ASM-NOT: sys memcpy_p
; ASM: ret
; O0-LABEL: call_barrier:
  %p0 = getelementptr i8, ptr %dst, i16 0
  store i16 257, ptr %p0, align 1
  %p1 = getelementptr i8, ptr %dst, i16 2
  store i16 2310, ptr %p1, align 1
  %p2 = getelementptr i8, ptr %dst, i16 4
  store i16 4363, ptr %p2, align 1
  %p3 = getelementptr i8, ptr %dst, i16 6
  store i16 6416, ptr %p3, align 1
  %p4 = getelementptr i8, ptr %dst, i16 8
  store i16 8469, ptr %p4, align 1
  %p5 = getelementptr i8, ptr %dst, i16 10
  store i16 10522, ptr %p5, align 1
  %p6 = getelementptr i8, ptr %dst, i16 12
  store i16 12575, ptr %p6, align 1
  %p7 = getelementptr i8, ptr %dst, i16 14
  store i16 14628, ptr %p7, align 1
  call void @observe(ptr %dst)
  %p8 = getelementptr i8, ptr %dst, i16 16
  store i16 16681, ptr %p8, align 1
  %p9 = getelementptr i8, ptr %dst, i16 18
  store i16 18734, ptr %p9, align 1
  %p10 = getelementptr i8, ptr %dst, i16 20
  store i16 20787, ptr %p10, align 1
  %p11 = getelementptr i8, ptr %dst, i16 22
  store i16 22840, ptr %p11, align 1
  %p12 = getelementptr i8, ptr %dst, i16 24
  store i16 24893, ptr %p12, align 1
  %p13 = getelementptr i8, ptr %dst, i16 26
  store i16 26946, ptr %p13, align 1
  %p14 = getelementptr i8, ptr %dst, i16 28
  store i16 28999, ptr %p14, align 1
  %p15 = getelementptr i8, ptr %dst, i16 30
  store i16 31052, ptr %p15, align 1
  ret void
}

define void @load_barrier(ptr %dst)  {
; IR-LABEL: define void @load_barrier(
; IR-NOT: call void @llvm.memcpy
; IR-NOT: call void @llvm.memset
; IR: store 
; IR: ret void
; ASM-LABEL: load_barrier:
; ASM-NOT: sys memcpy_p
; ASM: ret
; O0-LABEL: load_barrier:
  %p0 = getelementptr i8, ptr %dst, i16 0
  store i16 257, ptr %p0, align 1
  %p1 = getelementptr i8, ptr %dst, i16 2
  store i16 2310, ptr %p1, align 1
  %p2 = getelementptr i8, ptr %dst, i16 4
  store i16 4363, ptr %p2, align 1
  %p3 = getelementptr i8, ptr %dst, i16 6
  store i16 6416, ptr %p3, align 1
  %p4 = getelementptr i8, ptr %dst, i16 8
  store i16 8469, ptr %p4, align 1
  %p5 = getelementptr i8, ptr %dst, i16 10
  store i16 10522, ptr %p5, align 1
  %p6 = getelementptr i8, ptr %dst, i16 12
  store i16 12575, ptr %p6, align 1
  %p7 = getelementptr i8, ptr %dst, i16 14
  store i16 14628, ptr %p7, align 1
  %read = load volatile i8, ptr %dst
  %p8 = getelementptr i8, ptr %dst, i16 16
  store i16 16681, ptr %p8, align 1
  %p9 = getelementptr i8, ptr %dst, i16 18
  store i16 18734, ptr %p9, align 1
  %p10 = getelementptr i8, ptr %dst, i16 20
  store i16 20787, ptr %p10, align 1
  %p11 = getelementptr i8, ptr %dst, i16 22
  store i16 22840, ptr %p11, align 1
  %p12 = getelementptr i8, ptr %dst, i16 24
  store i16 24893, ptr %p12, align 1
  %p13 = getelementptr i8, ptr %dst, i16 26
  store i16 26946, ptr %p13, align 1
  %p14 = getelementptr i8, ptr %dst, i16 28
  store i16 28999, ptr %p14, align 1
  %p15 = getelementptr i8, ptr %dst, i16 30
  store i16 31052, ptr %p15, align 1
  ret void
}

define void @different_bases(ptr %dst)  {
; IR-LABEL: define void @different_bases(
; IR-NOT: call void @llvm.memcpy
; IR-NOT: call void @llvm.memset
; IR: store 
; IR: ret void
; ASM-LABEL: different_bases:
; ASM-NOT: sys memcpy_p
; ASM: ret
; O0-LABEL: different_bases:
  %p0 = getelementptr i8, ptr %dst, i16 0
  store i16 257, ptr %p0, align 1
  %p1 = getelementptr i8, ptr %dst, i16 2
  store i16 2310, ptr %p1, align 1
  %p2 = getelementptr i8, ptr %dst, i16 4
  store i16 4363, ptr %p2, align 1
  %p3 = getelementptr i8, ptr %dst, i16 6
  store i16 6416, ptr %p3, align 1
  %p4 = getelementptr i8, ptr %dst, i16 8
  store i16 8469, ptr %p4, align 1
  %p5 = getelementptr i8, ptr %dst, i16 10
  store i16 10522, ptr %p5, align 1
  %p6 = getelementptr i8, ptr %dst, i16 12
  store i16 12575, ptr %p6, align 1
  %p7 = getelementptr i8, ptr %dst, i16 14
  store i16 14628, ptr %p7, align 1
  store i8 17, ptr @ram, align 1
  %p8 = getelementptr i8, ptr %dst, i16 16
  store i16 16681, ptr %p8, align 1
  %p9 = getelementptr i8, ptr %dst, i16 18
  store i16 18734, ptr %p9, align 1
  %p10 = getelementptr i8, ptr %dst, i16 20
  store i16 20787, ptr %p10, align 1
  %p11 = getelementptr i8, ptr %dst, i16 22
  store i16 22840, ptr %p11, align 1
  %p12 = getelementptr i8, ptr %dst, i16 24
  store i16 24893, ptr %p12, align 1
  %p13 = getelementptr i8, ptr %dst, i16 26
  store i16 26946, ptr %p13, align 1
  %p14 = getelementptr i8, ptr %dst, i16 28
  store i16 28999, ptr %p14, align 1
  %p15 = getelementptr i8, ptr %dst, i16 30
  store i16 31052, ptr %p15, align 1
  ret void
}

define void @gaps(ptr %dst)  {
; IR-LABEL: define void @gaps(
; IR-NOT: call void @llvm.memcpy
; IR-NOT: call void @llvm.memset
; IR: store 
; IR: ret void
; ASM-LABEL: gaps:
; ASM-NOT: sys memcpy_p
; ASM: ret
; O0-LABEL: gaps:
  %p0 = getelementptr i8, ptr %dst, i16 0
  store i16 257, ptr %p0, align 1
  %p1 = getelementptr i8, ptr %dst, i16 4
  store i16 2310, ptr %p1, align 1
  %p2 = getelementptr i8, ptr %dst, i16 8
  store i16 4363, ptr %p2, align 1
  %p3 = getelementptr i8, ptr %dst, i16 12
  store i16 6416, ptr %p3, align 1
  %p4 = getelementptr i8, ptr %dst, i16 16
  store i16 8469, ptr %p4, align 1
  %p5 = getelementptr i8, ptr %dst, i16 20
  store i16 10522, ptr %p5, align 1
  %p6 = getelementptr i8, ptr %dst, i16 24
  store i16 12575, ptr %p6, align 1
  %p7 = getelementptr i8, ptr %dst, i16 28
  store i16 14628, ptr %p7, align 1
  %p8 = getelementptr i8, ptr %dst, i16 32
  store i16 16681, ptr %p8, align 1
  %p9 = getelementptr i8, ptr %dst, i16 36
  store i16 18734, ptr %p9, align 1
  %p10 = getelementptr i8, ptr %dst, i16 40
  store i16 20787, ptr %p10, align 1
  %p11 = getelementptr i8, ptr %dst, i16 44
  store i16 22840, ptr %p11, align 1
  %p12 = getelementptr i8, ptr %dst, i16 48
  store i16 24893, ptr %p12, align 1
  %p13 = getelementptr i8, ptr %dst, i16 52
  store i16 26946, ptr %p13, align 1
  %p14 = getelementptr i8, ptr %dst, i16 56
  store i16 28999, ptr %p14, align 1
  %p15 = getelementptr i8, ptr %dst, i16 60
  store i16 31052, ptr %p15, align 1
  ret void
}

define void @optnone(ptr %dst) noinline optnone {
; IR-LABEL: define void @optnone(
; IR-NOT: call void @llvm.memcpy
; IR-NOT: call void @llvm.memset
; IR: store 
; IR: ret void
; ASM-LABEL: optnone:
; ASM-NOT: sys memcpy_p
; ASM: ret
; O0-LABEL: optnone:
  %p0 = getelementptr i8, ptr %dst, i16 0
  store i16 257, ptr %p0, align 1
  %p1 = getelementptr i8, ptr %dst, i16 2
  store i16 2310, ptr %p1, align 1
  %p2 = getelementptr i8, ptr %dst, i16 4
  store i16 4363, ptr %p2, align 1
  %p3 = getelementptr i8, ptr %dst, i16 6
  store i16 6416, ptr %p3, align 1
  %p4 = getelementptr i8, ptr %dst, i16 8
  store i16 8469, ptr %p4, align 1
  %p5 = getelementptr i8, ptr %dst, i16 10
  store i16 10522, ptr %p5, align 1
  %p6 = getelementptr i8, ptr %dst, i16 12
  store i16 12575, ptr %p6, align 1
  %p7 = getelementptr i8, ptr %dst, i16 14
  store i16 14628, ptr %p7, align 1
  %p8 = getelementptr i8, ptr %dst, i16 16
  store i16 16681, ptr %p8, align 1
  %p9 = getelementptr i8, ptr %dst, i16 18
  store i16 18734, ptr %p9, align 1
  %p10 = getelementptr i8, ptr %dst, i16 20
  store i16 20787, ptr %p10, align 1
  %p11 = getelementptr i8, ptr %dst, i16 22
  store i16 22840, ptr %p11, align 1
  %p12 = getelementptr i8, ptr %dst, i16 24
  store i16 24893, ptr %p12, align 1
  %p13 = getelementptr i8, ptr %dst, i16 26
  store i16 26946, ptr %p13, align 1
  %p14 = getelementptr i8, ptr %dst, i16 28
  store i16 28999, ptr %p14, align 1
  %p15 = getelementptr i8, ptr %dst, i16 30
  store i16 31052, ptr %p15, align 1
  %p16 = getelementptr i8, ptr %dst, i16 32
  store i16 33105, ptr %p16, align 1
  %p17 = getelementptr i8, ptr %dst, i16 34
  store i16 35158, ptr %p17, align 1
  %p18 = getelementptr i8, ptr %dst, i16 36
  store i16 37211, ptr %p18, align 1
  %p19 = getelementptr i8, ptr %dst, i16 38
  store i16 39264, ptr %p19, align 1
  %p20 = getelementptr i8, ptr %dst, i16 40
  store i16 41317, ptr %p20, align 1
  %p21 = getelementptr i8, ptr %dst, i16 42
  store i16 43370, ptr %p21, align 1
  %p22 = getelementptr i8, ptr %dst, i16 44
  store i16 45423, ptr %p22, align 1
  %p23 = getelementptr i8, ptr %dst, i16 46
  store i16 47476, ptr %p23, align 1
  %p24 = getelementptr i8, ptr %dst, i16 48
  store i16 49529, ptr %p24, align 1
  %p25 = getelementptr i8, ptr %dst, i16 50
  store i16 51582, ptr %p25, align 1
  %p26 = getelementptr i8, ptr %dst, i16 52
  store i16 53635, ptr %p26, align 1
  %p27 = getelementptr i8, ptr %dst, i16 54
  store i16 55688, ptr %p27, align 1
  %p28 = getelementptr i8, ptr %dst, i16 56
  store i16 57741, ptr %p28, align 1
  %p29 = getelementptr i8, ptr %dst, i16 58
  store i16 59794, ptr %p29, align 1
  %p30 = getelementptr i8, ptr %dst, i16 60
  store i16 61847, ptr %p30, align 1
  %p31 = getelementptr i8, ptr %dst, i16 62
  store i16 63900, ptr %p31, align 1
  ret void
}

define void @partial_element_array(ptr %dst)  {
; IR-LABEL: define void @partial_element_array(
; IR-NOT: call void @llvm.memcpy
; IR-NOT: call void @llvm.memset
; IR: store 
; IR: ret void
; ASM-LABEL: partial_element_array:
; ASM-NOT: sys memcpy_p
; ASM: ret
; O0-LABEL: partial_element_array:
  %p0 = getelementptr i8, ptr %dst, i16 0
  store [32 x i7] zeroinitializer, ptr %p0, align 1
  ret void
}

define void @undefined(ptr %dst)  {
; IR-LABEL: define void @undefined(
; IR-NOT: call void @llvm.memcpy
; IR-NOT: call void @llvm.memset
; IR: store 
; IR: ret void
; ASM-LABEL: undefined:
; ASM-NOT: sys memcpy_p
; ASM: ret
; O0-LABEL: undefined:
  %p0 = getelementptr i8, ptr %dst, i16 0
  store i16 257, ptr %p0, align 1
  %p1 = getelementptr i8, ptr %dst, i16 2
  store i16 2310, ptr %p1, align 1
  %p2 = getelementptr i8, ptr %dst, i16 4
  store i16 4363, ptr %p2, align 1
  %p3 = getelementptr i8, ptr %dst, i16 6
  store i16 6416, ptr %p3, align 1
  %p4 = getelementptr i8, ptr %dst, i16 8
  store i16 8469, ptr %p4, align 1
  %p5 = getelementptr i8, ptr %dst, i16 10
  store i16 10522, ptr %p5, align 1
  %p6 = getelementptr i8, ptr %dst, i16 12
  store i16 12575, ptr %p6, align 1
  %p7 = getelementptr i8, ptr %dst, i16 14
  store i16 14628, ptr %p7, align 1
  %p8 = getelementptr i8, ptr %dst, i16 16
  store i16 undef, ptr %p8, align 1
  %p9 = getelementptr i8, ptr %dst, i16 18
  store i16 18734, ptr %p9, align 1
  %p10 = getelementptr i8, ptr %dst, i16 20
  store i16 20787, ptr %p10, align 1
  %p11 = getelementptr i8, ptr %dst, i16 22
  store i16 22840, ptr %p11, align 1
  %p12 = getelementptr i8, ptr %dst, i16 24
  store i16 24893, ptr %p12, align 1
  %p13 = getelementptr i8, ptr %dst, i16 26
  store i16 26946, ptr %p13, align 1
  %p14 = getelementptr i8, ptr %dst, i16 28
  store i16 28999, ptr %p14, align 1
  %p15 = getelementptr i8, ptr %dst, i16 30
  store i16 31052, ptr %p15, align 1
  ret void
}

define void @isolate_complex_caller(ptr %dst, i1 %flag) {
; IR-LABEL: define void @isolate_complex_caller(
; IR: call{{.*}} void @llvm.memcpy.p0.p1.i16({{.*}}i16 64, i1 false)
; ASM-LABEL: isolate_complex_caller:
; ASM-NOT: {{^[ \t]+call}}
; ASM: sys memcpy_p
; ASM-NOT: {{^[ \t]+call}}
; IR-NOT: store 
; IR: br i1 %flag
; O0-LABEL: isolate_complex_caller:
entry:
  %p0 = getelementptr i8, ptr %dst, i16 0
  store i16 257, ptr %p0, align 1
  %p1 = getelementptr i8, ptr %dst, i16 2
  store i16 2310, ptr %p1, align 1
  %p2 = getelementptr i8, ptr %dst, i16 4
  store i16 4363, ptr %p2, align 1
  %p3 = getelementptr i8, ptr %dst, i16 6
  store i16 6416, ptr %p3, align 1
  %p4 = getelementptr i8, ptr %dst, i16 8
  store i16 8469, ptr %p4, align 1
  %p5 = getelementptr i8, ptr %dst, i16 10
  store i16 10522, ptr %p5, align 1
  %p6 = getelementptr i8, ptr %dst, i16 12
  store i16 12575, ptr %p6, align 1
  %p7 = getelementptr i8, ptr %dst, i16 14
  store i16 14628, ptr %p7, align 1
  %p8 = getelementptr i8, ptr %dst, i16 16
  store i16 16681, ptr %p8, align 1
  %p9 = getelementptr i8, ptr %dst, i16 18
  store i16 18734, ptr %p9, align 1
  %p10 = getelementptr i8, ptr %dst, i16 20
  store i16 20787, ptr %p10, align 1
  %p11 = getelementptr i8, ptr %dst, i16 22
  store i16 22840, ptr %p11, align 1
  %p12 = getelementptr i8, ptr %dst, i16 24
  store i16 24893, ptr %p12, align 1
  %p13 = getelementptr i8, ptr %dst, i16 26
  store i16 26946, ptr %p13, align 1
  %p14 = getelementptr i8, ptr %dst, i16 28
  store i16 28999, ptr %p14, align 1
  %p15 = getelementptr i8, ptr %dst, i16 30
  store i16 31052, ptr %p15, align 1
  %p16 = getelementptr i8, ptr %dst, i16 32
  store i16 33105, ptr %p16, align 1
  %p17 = getelementptr i8, ptr %dst, i16 34
  store i16 35158, ptr %p17, align 1
  %p18 = getelementptr i8, ptr %dst, i16 36
  store i16 37211, ptr %p18, align 1
  %p19 = getelementptr i8, ptr %dst, i16 38
  store i16 39264, ptr %p19, align 1
  %p20 = getelementptr i8, ptr %dst, i16 40
  store i16 41317, ptr %p20, align 1
  %p21 = getelementptr i8, ptr %dst, i16 42
  store i16 43370, ptr %p21, align 1
  %p22 = getelementptr i8, ptr %dst, i16 44
  store i16 45423, ptr %p22, align 1
  %p23 = getelementptr i8, ptr %dst, i16 46
  store i16 47476, ptr %p23, align 1
  %p24 = getelementptr i8, ptr %dst, i16 48
  store i16 49529, ptr %p24, align 1
  %p25 = getelementptr i8, ptr %dst, i16 50
  store i16 51582, ptr %p25, align 1
  %p26 = getelementptr i8, ptr %dst, i16 52
  store i16 53635, ptr %p26, align 1
  %p27 = getelementptr i8, ptr %dst, i16 54
  store i16 55688, ptr %p27, align 1
  %p28 = getelementptr i8, ptr %dst, i16 56
  store i16 57741, ptr %p28, align 1
  %p29 = getelementptr i8, ptr %dst, i16 58
  store i16 59794, ptr %p29, align 1
  %p30 = getelementptr i8, ptr %dst, i16 60
  store i16 61847, ptr %p30, align 1
  %p31 = getelementptr i8, ptr %dst, i16 62
  store i16 63900, ptr %p31, align 1
  br i1 %flag, label %yes, label %no
yes:
  store volatile i8 7, ptr %dst
  ret void
no:
  ret void
}
