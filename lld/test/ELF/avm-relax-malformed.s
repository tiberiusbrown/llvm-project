# RUN: split-file %s %t
# RUN: yaml2obj %t/unsupported.yaml -o %t/unsupported.o
# RUN: not ld.lld %t/unsupported.o -o %t/unsupported.out 2>&1 | FileCheck %s --check-prefix=BAD-OP
# RUN: yaml2obj %t/truncated.yaml -o %t/truncated.o
# RUN: not ld.lld %t/truncated.o -o %t/truncated.out 2>&1 | FileCheck %s --check-prefix=TRUNCATED
# RUN: yaml2obj %t/marker-args.yaml -o %t/marker-args.o
# RUN: not ld.lld %t/marker-args.o -o %t/marker-args.out 2>&1 | FileCheck %s --check-prefix=MARKER-ARGS
# RUN: yaml2obj %t/missing-pair.yaml -o %t/missing-pair.o
# RUN: not ld.lld %t/missing-pair.o -o %t/missing-pair.out 2>&1 | FileCheck %s --check-prefix=MISSING-PAIR
# RUN: yaml2obj %t/wrong-field.yaml -o %t/wrong-field.o
# RUN: not ld.lld %t/wrong-field.o -o %t/wrong-field.out 2>&1 | FileCheck %s --check-prefix=WRONG-FIELD
# RUN: yaml2obj %t/duplicate-pair.yaml -o %t/duplicate-pair.o
# RUN: not ld.lld %t/duplicate-pair.o -o %t/duplicate-pair.out 2>&1 | FileCheck %s --check-prefix=DUPLICATE-PAIR
# RUN: yaml2obj %t/malformed-cond.yaml -o %t/malformed-cond.o
# RUN: not ld.lld %t/malformed-cond.o -o %t/malformed-cond.out 2>&1 | FileCheck %s --check-prefix=MALFORMED-COND
# RUN: yaml2obj %t/overlap.yaml -o %t/overlap.o
# RUN: not ld.lld %t/overlap.o -o %t/overlap.out 2>&1 | FileCheck %s --check-prefix=OVERLAP

# BAD-OP: R_AVM_RELAX is not attached to a supported AVM sequence
# TRUNCATED: truncated AVM relaxation sequence
# MARKER-ARGS: R_AVM_RELAX must use symbol zero and addend zero
# MISSING-PAIR: R_AVM_RELAX is not paired with exactly one R_AVM_FAR24
# WRONG-FIELD: R_AVM_FAR24 is at the wrong AVM relaxation field
# WRONG-FIELD: R_AVM_RELAX is not paired with exactly one R_AVM_FAR24
# DUPLICATE-PAIR: R_AVM_RELAX is not paired with exactly one R_AVM_FAR24
# MALFORMED-COND: malformed conditional AVM relaxation sequence
# OVERLAP: overlapping AVM relaxation sequences

#--- unsupported.yaml
--- !ELF
FileHeader: {Class: ELFCLASS32, Data: ELFDATA2LSB, Type: ET_REL, Machine: EM_AVM}
Sections:
  - Name: .text
    Type: SHT_PROGBITS
    Flags: [SHF_ALLOC, SHF_EXECINSTR]
    Content: E400
  - Name: .rela.text
    Type: SHT_RELA
    Link: .symtab
    Info: .text
    Relocations:
      - {Offset: 0x0, Type: R_AVM_RELAX}
Symbols:
  - {Name: .text, Type: STT_SECTION, Section: .text}

#--- truncated.yaml
--- !ELF
FileHeader: {Class: ELFCLASS32, Data: ELFDATA2LSB, Type: ET_REL, Machine: EM_AVM}
Sections:
  - Name: .text
    Type: SHT_PROGBITS
    Flags: [SHF_ALLOC, SHF_EXECINSTR]
    Content: E2
  - Name: .rela.text
    Type: SHT_RELA
    Link: .symtab
    Info: .text
    Relocations:
      - {Offset: 0x0, Type: R_AVM_RELAX}
      - {Offset: 0x1, Type: R_AVM_FAR24}
Symbols:
  - {Name: .text, Type: STT_SECTION, Section: .text}

#--- marker-args.yaml
--- !ELF
FileHeader: {Class: ELFCLASS32, Data: ELFDATA2LSB, Type: ET_REL, Machine: EM_AVM}
Sections:
  - Name: .text
    Type: SHT_PROGBITS
    Flags: [SHF_ALLOC, SHF_EXECINSTR]
    Content: E200000000
  - Name: .rela.text
    Type: SHT_RELA
    Link: .symtab
    Info: .text
    Relocations:
      - {Offset: 0x0, Symbol: target, Type: R_AVM_RELAX, Addend: 1}
      - {Offset: 0x1, Symbol: target, Type: R_AVM_FAR24}
Symbols:
  - {Name: .text, Type: STT_SECTION, Section: .text}
  - {Name: target, Type: STT_NOTYPE, Section: .text, Value: 0x0}

#--- missing-pair.yaml
--- !ELF
FileHeader: {Class: ELFCLASS32, Data: ELFDATA2LSB, Type: ET_REL, Machine: EM_AVM}
Sections:
  - Name: .text
    Type: SHT_PROGBITS
    Flags: [SHF_ALLOC, SHF_EXECINSTR]
    Content: E2000000
  - Name: .rela.text
    Type: SHT_RELA
    Link: .symtab
    Info: .text
    Relocations:
      - {Offset: 0x0, Type: R_AVM_RELAX}
Symbols:
  - {Name: .text, Type: STT_SECTION, Section: .text}

#--- wrong-field.yaml
--- !ELF
FileHeader: {Class: ELFCLASS32, Data: ELFDATA2LSB, Type: ET_REL, Machine: EM_AVM}
Sections:
  - Name: .text
    Type: SHT_PROGBITS
    Flags: [SHF_ALLOC, SHF_EXECINSTR]
    Content: E200000000
  - Name: .rela.text
    Type: SHT_RELA
    Link: .symtab
    Info: .text
    Relocations:
      - {Offset: 0x0, Type: R_AVM_RELAX}
      - {Offset: 0x2, Type: R_AVM_FAR24}
Symbols:
  - {Name: .text, Type: STT_SECTION, Section: .text}

#--- duplicate-pair.yaml
--- !ELF
FileHeader: {Class: ELFCLASS32, Data: ELFDATA2LSB, Type: ET_REL, Machine: EM_AVM}
Sections:
  - Name: .text
    Type: SHT_PROGBITS
    Flags: [SHF_ALLOC, SHF_EXECINSTR]
    Content: E2000000
  - Name: .rela.text
    Type: SHT_RELA
    Link: .symtab
    Info: .text
    Relocations:
      - {Offset: 0x0, Type: R_AVM_RELAX}
      - {Offset: 0x1, Type: R_AVM_FAR24}
      - {Offset: 0x1, Type: R_AVM_FAR24}
Symbols:
  - {Name: .text, Type: STT_SECTION, Section: .text}

#--- malformed-cond.yaml
--- !ELF
FileHeader: {Class: ELFCLASS32, Data: ELFDATA2LSB, Type: ET_REL, Machine: EM_AVM}
Sections:
  - Name: .text
    Type: SHT_PROGBITS
    Flags: [SHF_ALLOC, SHF_EXECINSTR]
    Content: D003E2000000
  - Name: .rela.text
    Type: SHT_RELA
    Link: .symtab
    Info: .text
    Relocations:
      - {Offset: 0x0, Type: R_AVM_RELAX}
      - {Offset: 0x3, Type: R_AVM_FAR24}
Symbols:
  - {Name: .text, Type: STT_SECTION, Section: .text}

#--- overlap.yaml
--- !ELF
FileHeader: {Class: ELFCLASS32, Data: ELFDATA2LSB, Type: ET_REL, Machine: EM_AVM}
Sections:
  - Name: .text
    Type: SHT_PROGBITS
    Flags: [SHF_ALLOC, SHF_EXECINSTR]
    Content: D004E2E20000
  - Name: .rela.text
    Type: SHT_RELA
    Link: .symtab
    Info: .text
    Relocations:
      - {Offset: 0x0, Type: R_AVM_RELAX}
      - {Offset: 0x2, Type: R_AVM_RELAX}
      - {Offset: 0x3, Type: R_AVM_FAR24}
Symbols:
  - {Name: .text, Type: STT_SECTION, Section: .text}
