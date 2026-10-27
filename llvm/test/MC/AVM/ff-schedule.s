# RUN: llvm-mca -mtriple=avm -mcpu=avm1 -iterations=1 \
# RUN:   -all-views=0 -instruction-info < %s | FileCheck %s

# Fixed-register instructions have no explicit MC operands. Their implicit
# definitions/uses must still be available to MCA, including q2/r4 aliases.
fadd
fsub
fmul
fdiv
s16tof
u16tof
ftos16
ftou16
s32tof
u32tof
ftos32
ftou32
fcmp
fclass
fneg q2
fabs q3

# CHECK:      Instruction Info:
# CHECK:      1      156   156.00                      fadd
# CHECK-NEXT: 1      156   156.00                      fsub
# CHECK-NEXT: 1      192   192.00                      fmul
# CHECK-NEXT: 1      529   529.00                      fdiv
# CHECK-NEXT: 1      125   125.00                      s16tof
# CHECK-NEXT: 1      123   123.00                      u16tof
# CHECK-NEXT: 1      112   112.00                      ftos16
# CHECK-NEXT: 1      102   102.00                      ftou16
# CHECK-NEXT: 1      122   122.00                      s32tof
# CHECK-NEXT: 1      122   122.00                      u32tof
# CHECK-NEXT: 1      115   115.00                      ftos32
# CHECK-NEXT: 1      105   105.00                      ftou32
# CHECK-NEXT: 1      83    83.00                       fcmp
# CHECK-NEXT: 1      110   110.00                      fclass
# CHECK-NEXT: 1      38    38.00                       fneg q2
# CHECK-NEXT: 1      38    38.00                       fabs q3
