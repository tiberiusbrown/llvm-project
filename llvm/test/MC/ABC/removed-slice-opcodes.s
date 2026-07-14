# RUN: not llvm-mc -triple=abc -filetype=obj %s -o %t 2>&1 | FileCheck %s

aslc    1
pslc    1

# CHECK: error:
