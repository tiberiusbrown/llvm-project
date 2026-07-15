# RUN: not grep -nE "BEQ_SHORT|BNE_SHORT|CPC16|ADC16|SBC16|MTPB|MFPB|LDPBI|beq\\.s|bne\\.s|cpc16|ldpbi" %S/../../../lib/Target/AVM
# RUN: grep -n "R_AVM_PCREL16" %S/../../../lib/Target/AVM/MCTargetDesc/AVMAsmBackend.cpp

# The command above is intentionally source-only: negative-test input may
# name removed mnemonics, but no active AVM implementation may define, parse,
# encode, print, or decode them.
