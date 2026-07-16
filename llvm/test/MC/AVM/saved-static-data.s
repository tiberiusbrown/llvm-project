# RUN: llvm-mc -triple=avm -filetype=obj %s -o %t.o
# RUN: llvm-readobj --file-headers --sections %t.o | FileCheck %s --check-prefix=SECTIONS
# RUN: llvm-objdump -s -j .saved -j .saved.config -j .data -j .data.vars %t.o | FileCheck %s --check-prefix=CONTENTS

.saved
.zero 16
.byte 1
.zero 3
.byte 5

.section .saved.config,"aw",@progbits
.zero 16

.data
.zero 16

.section .data.vars,"aw",@progbits
.byte 7

# SECTIONS: Flags [ (0x1)
# SECTIONS: Name: .saved
# SECTIONS: Type: SHT_PROGBITS
# SECTIONS: Flags [ (0x20000003)
# SECTIONS: Size: 21
# SECTIONS: Name: .saved.config
# SECTIONS: Type: SHT_PROGBITS
# SECTIONS: Flags [ (0x20000003)
# SECTIONS: Size: 16
# SECTIONS: Name: .data
# SECTIONS: Type: SHT_PROGBITS
# SECTIONS: Flags [ (0x20000003)
# SECTIONS: Size: 16
# SECTIONS: Name: .data.vars
# SECTIONS: Type: SHT_PROGBITS
# SECTIONS: Flags [ (0x20000003)

# CONTENTS: Contents of section .saved:
# CONTENTS: 0000 00000000 00000000 00000000 00000000
# CONTENTS: 0010 01000000 05
# CONTENTS: Contents of section .saved.config:
# CONTENTS: 0000 00000000 00000000 00000000 00000000
# CONTENTS: Contents of section .data:
# CONTENTS: 0000 00000000 00000000 00000000 00000000
# CONTENTS: Contents of section .data.vars:
# CONTENTS: 0000 07
