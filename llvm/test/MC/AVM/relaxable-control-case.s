# RUN: llvm-mc -triple=avm -show-encoding %s | FileCheck %s

JMP target
CALL target
BR.EQ target
BR.NE target
BR.ULT target
BR.UGE target
BR.SLT target
BR.SGE target

# CHECK: jmp target{{.*}}[0xe2,B,B,B]
# CHECK: call target{{.*}}[0xe3,B,B,B]
# CHECK: br.eq target{{.*}}[0xd1,0x04,0xe2,B,B,B]
# CHECK: br.ne target{{.*}}[0xd0,0x04,0xe2,B,B,B]
# CHECK: br.ult target{{.*}}[0xd8,0x04,0xe2,B,B,B]
# CHECK: br.uge target{{.*}}[0xd2,0x04,0xe2,B,B,B]
# CHECK: br.slt target{{.*}}[0xd9,0x04,0xe2,B,B,B]
# CHECK: br.sge target{{.*}}[0xd3,0x04,0xe2,B,B,B]
