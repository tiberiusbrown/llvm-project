# RUN: not llvm-mc -triple=avm %s 2>&1 | FileCheck %s

sys no_such_service

# CHECK: error: unknown AVM system function 'no_such_service'
