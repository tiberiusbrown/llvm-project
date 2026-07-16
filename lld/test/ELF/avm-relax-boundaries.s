# RUN: split-file %s %t
# RUN: llvm-mc -triple=avm -filetype=obj %t/jump.s -o %t/jump.o
# RUN: llvm-mc -triple=avm -filetype=obj %t/call.s -o %t/call.o
# RUN: llvm-mc -triple=avm -filetype=obj %t/cond.s -o %t/cond.o
# RUN: ld.lld -Ttext=0x10000 --defsym=target=0xff82 %t/jump.o -o %t/j8n
# RUN: llvm-objdump -s -j .text %t/j8n | FileCheck %s --check-prefix=J8N
# RUN: ld.lld -Ttext=0x10000 --defsym=target=0x10081 %t/jump.o -o %t/j8p
# RUN: llvm-objdump -s -j .text %t/j8p | FileCheck %s --check-prefix=J8P
# RUN: ld.lld -Ttext=0x10000 --defsym=target=0xff81 %t/jump.o -o %t/j16n
# RUN: llvm-objdump -s -j .text %t/j16n | FileCheck %s --check-prefix=J16N
# RUN: ld.lld -Ttext=0x10000 --defsym=target=0x10082 %t/jump.o -o %t/j16p
# RUN: llvm-objdump -s -j .text %t/j16p | FileCheck %s --check-prefix=J16P
# RUN: ld.lld -Ttext=0x10000 --defsym=target=0x8003 %t/jump.o -o %t/j16min
# RUN: llvm-objdump -s -j .text %t/j16min | FileCheck %s --check-prefix=J16MIN
# RUN: ld.lld -Ttext=0x10000 --defsym=target=0x18002 %t/jump.o -o %t/j16max
# RUN: llvm-objdump -s -j .text %t/j16max | FileCheck %s --check-prefix=J16MAX
# RUN: ld.lld -Ttext=0x10000 --defsym=target=0x8002 %t/jump.o -o %t/jfarmin
# RUN: llvm-objdump -s -j .text %t/jfarmin | FileCheck %s --check-prefix=JFARMIN
# RUN: ld.lld -Ttext=0x10000 --defsym=target=0x18003 %t/jump.o -o %t/jfarmax
# RUN: llvm-objdump -s -j .text %t/jfarmax | FileCheck %s --check-prefix=JFARMAX
# RUN: ld.lld -Ttext=0x10000 --defsym=target=0xff82 %t/call.o -o %t/c8n
# RUN: llvm-objdump -s -j .text %t/c8n | FileCheck %s --check-prefix=C8N
# RUN: ld.lld -Ttext=0x10000 --defsym=target=0x10081 %t/call.o -o %t/c8p
# RUN: llvm-objdump -s -j .text %t/c8p | FileCheck %s --check-prefix=C8P
# RUN: ld.lld -Ttext=0x10000 --defsym=target=0xff81 %t/call.o -o %t/c16n
# RUN: llvm-objdump -s -j .text %t/c16n | FileCheck %s --check-prefix=C16N
# RUN: ld.lld -Ttext=0x10000 --defsym=target=0x10082 %t/call.o -o %t/c16p
# RUN: llvm-objdump -s -j .text %t/c16p | FileCheck %s --check-prefix=C16P
# RUN: ld.lld -Ttext=0x10000 --defsym=target=0x8003 %t/call.o -o %t/c16min
# RUN: llvm-objdump -s -j .text %t/c16min | FileCheck %s --check-prefix=C16MIN
# RUN: ld.lld -Ttext=0x10000 --defsym=target=0x18002 %t/call.o -o %t/c16max
# RUN: llvm-objdump -s -j .text %t/c16max | FileCheck %s --check-prefix=C16MAX
# RUN: ld.lld -Ttext=0x10000 --defsym=target=0x8002 %t/call.o -o %t/cfarmin
# RUN: llvm-objdump -s -j .text %t/cfarmin | FileCheck %s --check-prefix=CFARMIN
# RUN: ld.lld -Ttext=0x10000 --defsym=target=0x18003 %t/call.o -o %t/cfarmax
# RUN: llvm-objdump -s -j .text %t/cfarmax | FileCheck %s --check-prefix=CFARMAX
# RUN: ld.lld -Ttext=0x10000 --defsym=target=0x10081 %t/cond.o -o %t/cond8
# RUN: llvm-objdump -s -j .text %t/cond8 | FileCheck %s --check-prefix=COND8
# RUN: ld.lld -Ttext=0x10000 --defsym=target=0xff81 %t/cond.o -o %t/cond16n
# RUN: llvm-objdump -s -j .text %t/cond16n | FileCheck %s --check-prefix=COND16N
# RUN: ld.lld -Ttext=0x10000 --defsym=target=0x10082 %t/cond.o -o %t/cond16p
# RUN: llvm-objdump -s -j .text %t/cond16p | FileCheck %s --check-prefix=COND16P
# RUN: ld.lld -Ttext=0x10000 --defsym=target=0x8004 %t/cond.o -o %t/condfarmin
# RUN: llvm-objdump -s -j .text %t/condfarmin | FileCheck %s --check-prefix=CONDFARMIN
# RUN: ld.lld -Ttext=0x10000 --defsym=target=0x18005 %t/cond.o -o %t/condfarmax
# RUN: llvm-objdump -s -j .text %t/condfarmax | FileCheck %s --check-prefix=CONDFARMAX

# J8N: 10000 d480
# J8P: 10000 d47f
# J16N: 10000 e07eff
# J16P: 10000 e07f00
# J16MIN: 10000 e00080
# J16MAX: 10000 e0ff7f
# JFARMIN: 10000 e2028000
# JFARMAX: 10000 e2038001
# C8N: 10000 d580
# C8P: 10000 d57f
# C16N: 10000 e17eff
# C16P: 10000 e17f00
# C16MIN: 10000 e10080
# C16MAX: 10000 e1ff7f
# CFARMIN: 10000 e3028000
# CFARMAX: 10000 e3038001
# COND8: 10000 d07fd17d d27bd879 d377d975
# COND16N: 10000 da7effdb 7bffdc78 ffdd75ff de72ffdf
# COND16N-NEXT: 10010 6fff
# COND16P: 10000 da7f00d1 7dd27bd8 79d377d9 75
# CONDFARMIN: 10000 da0180d0 04e20480 00d804e2 048000d2
# CONDFARMIN-NEXT: 10010 04e20480 00d904e2 048000d3 04e20480
# CONDFARMIN-NEXT: 10020 00
# CONDFARMAX: 10000 d104e205 8001dbfc 7fdcf97f ddf67fde
# CONDFARMAX-NEXT: 10010 f37fdff0 7f

#--- jump.s
.text
jmp target

#--- call.s
.text
call target

#--- cond.s
.text
breq target
brne target
brult target
bruge target
brslt target
brsge target
