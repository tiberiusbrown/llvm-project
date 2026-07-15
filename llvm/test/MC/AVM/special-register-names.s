# RUN: llvm-mc -triple=avm-unknown-arduboyfx -filetype=null %s

# CFI directives exercise the target's generic architectural-register parser
# without requiring an instruction family with explicit special-state operands.
.cfi_startproc
.cfi_same_value sp
.cfi_same_value pc
.cfi_same_value cc
.cfi_endproc
