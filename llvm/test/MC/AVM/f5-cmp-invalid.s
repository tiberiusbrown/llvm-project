# RUN: not llvm-mc -triple=avm -filetype=null %s 2>&1 | FileCheck %s

cmp c0,r0
cmp q0,r0
cmp sp,r0
cmp pc,r0
cmp r0,c0
cmp r0,q0
cmp r0,sp
cmp r0,cc
cmp r4,r4
cmp r4,r7
cmp r5,r6
cmp r7,r4
cmp r0,1
cmp 1,r0
cmp r0,[r1]
cmp [r0],r1
cmp r0,symbol
cmp
cmp r0
cmp r0,r1,r2

# CHECK-DAG: error: expected compact register c0-c3
# CHECK-DAG: error: expected compact register c0-c3
# CHECK-DAG: error: expected compact register c0-c3
# CHECK-DAG: error: expected compact register c0-c3
# CHECK-DAG: error: expected full register r0-r7
# CHECK-DAG: error: expected full register r0-r7
# CHECK-DAG: error: expected full register r0-r7
# CHECK-DAG: error: expected full register r0-r7
# CHECK-DAG: error: cmp full-register pairing is not encodable
# CHECK-DAG: error: cmp full-register pairing is not encodable
# CHECK-DAG: error: cmp full-register pairing is not encodable
# CHECK-DAG: error: cmp full-register pairing is not encodable
# CHECK-DAG: error: expected AVM register
# CHECK-DAG: error: expected AVM register
# CHECK-DAG: error: expected comma
# CHECK-DAG: error: unexpected token after AVM instruction
