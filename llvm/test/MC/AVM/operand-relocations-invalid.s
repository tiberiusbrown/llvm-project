# RUN: not llvm-mc -triple=avm-unknown-arduboyfx -filetype=obj %s -o /dev/null 2>&1 | FileCheck %s

ldi8 r0, prog_lo16(program_symbol)
cmpi8 r0, program_symbol
ldi16 r0, prog_hi8(program_symbol)
addi16 r0, prog_hi8(program_symbol)
ldm16 r0, prog_lo16(program_symbol)
breq prog_hi8(program_symbol)
jmp16 prog_lo16(program_symbol)
jmpf prog_lo16(program_symbol)
sys prog_hi8(program_symbol)
adjsp data_symbol
ld8 r0, [r1+data_symbol]
ldsp8 r0, [sp+data_symbol]
beq.s program_symbol

ldi8 r0, 256
ldi16 r0, 65536
ldm16 r0, 65536
jmp16 65536
jmpf 16777216

# CHECK: error: LDI8 immediate requires prog_hi8(expression), not prog_lo16
# CHECK: error: symbolic CMPI8 immediate requires an AVM address modifier
# CHECK-COUNT-2: error: 16-bit instruction immediate requires prog_lo16(expression), not prog_hi8(expression)
# CHECK: error: AVM address modifiers are not valid for direct data-space address
# CHECK: error: AVM address modifiers are not valid for PC-relative branch target
# CHECK: error: AVM address modifiers are not valid for PC-relative control target
# CHECK: error: AVM address modifiers are not valid for far program target
# CHECK: error: SYS service must be an absolute immediate
# CHECK: error: ADJSP immediate must be an absolute immediate
# CHECK: error: memory displacement must be an absolute immediate
# CHECK: error: stack offset must be an absolute immediate
# CHECK: error: short branch displacement must be an absolute immediate
# CHECK: error: LDI8 immediate is out of unsigned 8-bit range
# CHECK: error: 16-bit immediate is out of range
# CHECK: error: direct data-space address is out of range
# CHECK: error: relative control displacement is out of signed 16-bit range
# CHECK: error: far target is out of 24-bit range
