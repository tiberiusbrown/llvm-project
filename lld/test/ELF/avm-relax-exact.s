# RUN: split-file %s %t
# RUN: llvm-mc -triple=avm -filetype=obj %t/exact.s -o %t/exact.o
# RUN: ld.lld -Ttext=0x10000 --defsym=target=0x10020 %t/exact.o -o %t/exact.out
# RUN: llvm-objdump -s -j .text %t/exact.out | FileCheck %s --check-prefix=EXACT
# RUN: llvm-mc -triple=avm -filetype=obj %t/short-overflow.s -o %t/short-overflow.o
# RUN: not ld.lld -Ttext=0x10000 --defsym=target=0x10100 %t/short-overflow.o -o %t/short-overflow.out 2>&1 | FileCheck %s --check-prefix=SHORT-ERR
# RUN: llvm-mc -triple=avm -filetype=obj %t/long-overflow.s -o %t/long-overflow.o
# RUN: not ld.lld -Ttext=0x10000 --defsym=target=0x18006 %t/long-overflow.o -o %t/long-overflow.out 2>&1 | FileCheck %s --check-prefix=LONG-ERR

# EXACT: 10000 e2200001 e3200001 e01500e1 1200d010
# EXACT-NEXT: 10010 d10ed20c d80ad308 d906d404 d502
# SHORT-ERR-COUNT-3: relocation R_AVM_PCREL8 out of range
# LONG-ERR-COUNT-2: relocation R_AVM_PCREL16 out of range

#--- exact.s
.text
jmpf target
callf target
jmp16 target
call16 target
breq target
brne target
brult target
bruge target
brslt target
brsge target
jmp8 target
call8 target

#--- short-overflow.s
.text
jmp8 target
call8 target
breq target

#--- long-overflow.s
.text
jmp16 target
call16 target
