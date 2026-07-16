# RUN: split-file %s %t
# RUN: llvm-mc -triple=avm -filetype=obj %t/cond.s -o %t/cond.o
# RUN: ld.lld -Ttext=0x10000 --defsym=eq=0xff82 --defsym=ne=0xff84 --defsym=ult=0xff86 --defsym=uge=0xff88 --defsym=slt=0xff8a --defsym=sge=0xff8c %t/cond.o -o %t/short-neg.out
# RUN: llvm-objdump -s -j .text %t/short-neg.out | FileCheck %s --check-prefix=SHORT-NEG
# RUN: ld.lld -Ttext=0x10000 --defsym=eq=0x10081 --defsym=ne=0x10083 --defsym=ult=0x10085 --defsym=uge=0x10087 --defsym=slt=0x10089 --defsym=sge=0x1008b %t/cond.o -o %t/short-pos.out
# RUN: llvm-objdump -s -j .text %t/short-pos.out | FileCheck %s --check-prefix=SHORT-POS
# RUN: ld.lld -Ttext=0x10000 --defsym=eq=0xff81 --defsym=ne=0xff86 --defsym=ult=0xff8b --defsym=uge=0xff90 --defsym=slt=0xff95 --defsym=sge=0xff9a %t/cond.o -o %t/long-neg.out
# RUN: llvm-objdump -s -j .text %t/long-neg.out | FileCheck %s --check-prefix=LONG-NEG
# RUN: ld.lld -Ttext=0x10000 --defsym=eq=0x10082 --defsym=ne=0x10087 --defsym=ult=0x1008c --defsym=uge=0x10091 --defsym=slt=0x10096 --defsym=sge=0x1009b %t/cond.o -o %t/long-pos.out
# RUN: llvm-objdump -s -j .text %t/long-pos.out | FileCheck %s --check-prefix=LONG-POS
# RUN: ld.lld -Ttext=0x10000 --defsym=eq=0x8005 --defsym=ne=0x800a --defsym=ult=0x800f --defsym=uge=0x8014 --defsym=slt=0x8019 --defsym=sge=0x801e %t/cond.o -o %t/long-min.out
# RUN: llvm-objdump -s -j .text %t/long-min.out | FileCheck %s --check-prefix=LONG-MIN
# RUN: ld.lld -Ttext=0x10000 --defsym=eq=0x18004 --defsym=ne=0x18009 --defsym=ult=0x1800e --defsym=uge=0x18013 --defsym=slt=0x18018 --defsym=sge=0x1801d %t/cond.o -o %t/long-max.out
# RUN: llvm-objdump -s -j .text %t/long-max.out | FileCheck %s --check-prefix=LONG-MAX
# RUN: ld.lld -Ttext=0x10000 --defsym=eq=0x8004 --defsym=ne=0x800a --defsym=ult=0x8010 --defsym=uge=0x8016 --defsym=slt=0x801c --defsym=sge=0x8022 %t/cond.o -o %t/far-min.out
# RUN: llvm-objdump -s -j .text %t/far-min.out | FileCheck %s --check-prefix=FAR-MIN
# RUN: ld.lld -Ttext=0x10000 --defsym=eq=0x18005 --defsym=ne=0x1800b --defsym=ult=0x18011 --defsym=uge=0x18017 --defsym=slt=0x1801d --defsym=sge=0x18023 %t/cond.o -o %t/far-max.out
# RUN: llvm-objdump -s -j .text %t/far-max.out | FileCheck %s --check-prefix=FAR-MAX

# SHORT-NEG: 10000 d080d180 d280d880 d380d980
# SHORT-POS: 10000 d07fd17f d27fd87f d37fd97f
# LONG-NEG: 10000 da7effd1 81d284d8 87d38ad9 8d
# LONG-POS: 10000 da7f00db 8100dc83 00dd8500 de8700df
# LONG-POS-NEXT: 10010 8900
# LONG-MIN: 10000 da0280db 0480dc06 80dd0880 de0a80df
# LONG-MIN-NEXT: 10010 0c80
# LONG-MAX: 10000 d104e204 8001d004 e2098001 dcff7fd2
# LONG-MAX-NEXT: 10010 04e21380 01d904e2 188001df ff7f
# FAR-MIN: 10000 da0180db 0480dc07 80dd0a80 de0d80df
# FAR-MIN-NEXT: 10010 1080
# FAR-MAX: 10000 d104e205 8001d004 e20b8001 d804e211
# FAR-MAX-NEXT: 10010 8001d204 e2178001 d904e21d 8001d304
# FAR-MAX-NEXT: 10020 e2238001

#--- cond.s
.text
breq eq
brne ne
brult ult
bruge uge
brslt slt
brsge sge
