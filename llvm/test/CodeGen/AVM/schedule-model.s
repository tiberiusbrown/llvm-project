# RUN: llvm-mca -mtriple=avm -mcpu=avm1 -mattr=+v1 -iterations=1 \
# RUN:   -all-views=0 -summary-view -instruction-info < %s | FileCheck %s

# The AVM interpreter executes on one serial resource. This sequence covers
# compact, full, dense, cold, program-memory, branch, divide, floating, and
# system-service schedule classes. Its measured costs sum to 1022 cycles.

add r4, r5
add r0, r1
ld16 r4, [r5]
ld16 r0, [r5]
ld16 r4, [r0]
ld16 r0, [r0]
ldp8u r4, [q2]
breq8 0
udiv16 r4, r5
fadd q2, q3
sys debug_putc

# CHECK:      Instructions:      11
# CHECK:      Block RThroughput: 1022.0
# CHECK:      Instruction Info:
# CHECK:      1      17    17.00                 U     add r4, r5
# CHECK-NEXT: 1      38    38.00                 U     add r0, r1
# CHECK-NEXT: 1      18    18.00   *             U     ld16 r4, [r5]
# CHECK-NEXT: 1      39    39.00   *             U     ld16 r0, [r5]
# CHECK-NEXT: 1      71    71.00   *             U     ld16 r4, [r0]
# CHECK-NEXT: 1      71    71.00   *             U     ld16 r0, [r0]
# CHECK-NEXT: 1      290   290.00  *             U     ldp8u r4, [q2]
# CHECK-NEXT: 1      35    35.00                 U     breq8 0
# CHECK-NEXT: 1      219   219.00                U     udiv16 r4, r5
# CHECK-NEXT: 1      190   190.00                U     fadd q2, q3
# CHECK-NEXT: 1      34    34.00                 U     sys debug_putc
