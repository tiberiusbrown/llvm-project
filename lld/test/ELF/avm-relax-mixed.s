# RUN: llvm-mc -triple=avm -filetype=obj %s -o %t.o
# RUN: ld.lld -Ttext=0x10000 --defsym=j8=0x10020 --defsym=j16=0x11000 --defsym=jfar=0x20000 --defsym=c8=0x10020 --defsym=c16=0x11000 --defsym=cfar=0x20000 --defsym=cdirect=0x10020 --defsym=clong=0x11000 --defsym=cfarcond=0x20000 %t.o -o %t.out
# RUN: llvm-readobj --sections %t.out | FileCheck %s --check-prefix=SIZE
# RUN: llvm-objdump -s -j .text %t.out | FileCheck %s --check-prefix=BYTES

# The maximal 46-byte input loses 13 bytes: 2+1 for JMP, 2+1 for CALL,
# and 4+3 for conditional sites. Exact instructions and trailing data remain.
# SIZE: Name: .text
# SIZE: Size: 33
# BYTES: 10000 d41ee0fb 0fe20000 02d515e1 f20fe300
# BYTES-NEXT: 10010 0002d00c dbe90fd8 04e20000 02d401aa
# BYTES-NEXT: 10020 bb

.text
jmp j8
jmp j16
jmp jfar
call c8
call c16
call cfar
breq cdirect
brne clong
brult cfarcond
jmp8 j8
.byte 0xaa, 0xbb
