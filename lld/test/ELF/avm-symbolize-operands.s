# RUN: llvm-mc -triple=avm -filetype=obj %s -o %t.o
# RUN: ld.lld -e entry -Ttext=0x10000 %t.o -o %t
# RUN: llvm-objdump -d --symbolize-operands %t | FileCheck %s --check-prefix=SYM
# RUN: llvm-objdump -d --no-symbolize-operands %t | FileCheck %s --check-prefix=NO-SYM

.text
.globl entry
.type entry,@function
entry:
  breq8 target
  brne8 target
  brult8 target
  bruge8 target
  brslt8 target
  brsge8 target
  jmp8 target
  call8 target
  breq16 target
  brne16 target
  brult16 target
  bruge16 target
  brslt16 target
  brsge16 target
  jmp16 target
  call16 target
  jmpf target
  callf target
back:
  call8 back
  jmp16 back
  ret

.globl target
.type target,@function
target:
  ret
  ret

.globl function
.type function,@function
function:
  ret
  ret
  call8 function+1
  jmp8 function+1
  breq8 function+1
  call16 function+1
  jmp16 function+1
  breq16 function+1
  callf function+1
  jmpf function+1

# SYM: breq8 target
# SYM: brne8 target
# SYM: brult8 target
# SYM: bruge8 target
# SYM: brslt8 target
# SYM: brsge8 target
# SYM: jmp8 target
# SYM: call8 target
# SYM: breq16 target
# SYM: brne16 target
# SYM: brult16 target
# SYM: bruge16 target
# SYM: brslt16 target
# SYM: brsge16 target
# SYM: jmp16 target
# SYM: call16 target
# SYM: jmpf target
# SYM: callf target
# SYM: call8 back
# SYM: jmp16 back
# SYM: call8 function+1
# SYM: jmp8 function+1
# SYM: breq8 function+1
# SYM: call16 function+1
# SYM: jmp16 function+1
# SYM: breq16 function+1
# SYM: callf function+1
# SYM: jmpf function+1

# NO-SYM: call8 0x{{[0-9a-f]+}}
