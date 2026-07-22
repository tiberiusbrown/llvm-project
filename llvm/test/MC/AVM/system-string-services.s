# RUN: llvm-mc -triple=avm -show-encoding %s | FileCheck %s --check-prefix=ENCODING
# RUN: llvm-mc -triple=avm -filetype=obj %s -o %t.o
# RUN: llvm-objdump -d --no-show-raw-insn %t.o | FileCheck %s --check-prefix=DISASM

sys memcmp_p
sys strcmp_p
sys strlen_p
sys strncpy_p
sys strncat_p
sys memcmp
sys strcmp
sys strlen
sys strncpy
sys strncat

# ENCODING: sys memcmp_p{{.*}}encoding: [0xd7,0x13]
# ENCODING: sys strcmp_p{{.*}}encoding: [0xd7,0x14]
# ENCODING: sys strlen_p{{.*}}encoding: [0xd7,0x15]
# ENCODING: sys strncpy_p{{.*}}encoding: [0xd7,0x16]
# ENCODING: sys strncat_p{{.*}}encoding: [0xd7,0x17]
# ENCODING: sys memcmp{{.*}}encoding: [0xd7,0x18]
# ENCODING: sys strcmp{{.*}}encoding: [0xd7,0x19]
# ENCODING: sys strlen{{.*}}encoding: [0xd7,0x1a]
# ENCODING: sys strncpy{{.*}}encoding: [0xd7,0x1b]
# ENCODING: sys strncat{{.*}}encoding: [0xd7,0x1c]

# DISASM:      sys memcmp_p
# DISASM-NEXT: sys strcmp_p
# DISASM-NEXT: sys strlen_p
# DISASM-NEXT: sys strncpy_p
# DISASM-NEXT: sys strncat_p
# DISASM-NEXT: sys memcmp
# DISASM-NEXT: sys strcmp
# DISASM-NEXT: sys strlen
# DISASM-NEXT: sys strncpy
# DISASM-NEXT: sys strncat
