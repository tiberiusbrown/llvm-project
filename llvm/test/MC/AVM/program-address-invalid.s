# RUN: not llvm-mc -triple=avm-unknown-arduboyfx -filetype=obj %s -o %t.o 2>&1 | FileCheck %s

ldpbi program_data
ldpbi prog_lo16(program_data)
ldi16 r6, prog_hi8(program_data)
addi16 r6, prog_lo16(program_data)
ldpbi prog_hi8(0x1000000)
ldi16 r6, prog_lo16(-1)

# CHECK: error: symbolic LDPBI immediate requires prog_hi8(expression)
# CHECK: error: symbolic LDPBI immediate requires prog_hi8(expression)
# CHECK: error: symbolic LDI16 immediate requires prog_lo16(expression)
# CHECK: error: prog_lo16 is valid only as an LDI16 immediate
# CHECK: error: program address is out of 24-bit range
# CHECK: error: program address is out of 24-bit range
