//===-- ABCELFObjectWriter.cpp - ABC ELF writer ---------------------------===//

#include "ABCFixupKinds.h"
#include "ABCMCTargetDesc.h"
#include "llvm/BinaryFormat/ELF.h"
#include "llvm/MC/MCELFObjectWriter.h"
#include "llvm/MC/MCFixup.h"
#include "llvm/MC/MCObjectWriter.h"
#include "llvm/MC/MCValue.h"
#include "llvm/Support/ErrorHandling.h"

using namespace llvm;

namespace {
class ABCELFObjectWriter : public MCELFObjectTargetWriter {
public:
  ABCELFObjectWriter(uint8_t OSABI)
      : MCELFObjectTargetWriter(false, OSABI, ELF::EM_ABC, true) {}

  unsigned getRelocType(const MCFixup &Fixup, const MCValue &Target,
                        bool IsPCRel) const override {
    switch (Fixup.getKind()) {
    case FK_Data_1:
    case ABC::fixup_abc_8:
      return ELF::R_ABC_8;
    case FK_Data_2:
    case ABC::fixup_abc_16:
      return ELF::R_ABC_16;
    case ABC::fixup_abc_24:
      return ELF::R_ABC_24;
    case FK_Data_4:
    case ABC::fixup_abc_32:
      return ELF::R_ABC_32;
    case ABC::fixup_abc_prog24:
      return ELF::R_ABC_PROG24;
    case ABC::fixup_abc_global16_tagged:
      return ELF::R_ABC_GLOBAL16_TAGGED;
    case ABC::fixup_abc_global8:
      return ELF::R_ABC_GLOBAL8;
    case ABC::fixup_abc_branch8:
      return ELF::R_ABC_BRANCH8;
    case ABC::fixup_abc_branch16:
      return ELF::R_ABC_BRANCH16;
    case ABC::fixup_abc_call24:
      return ELF::R_ABC_CALL24;
    default:
      llvm_unreachable("invalid ABC fixup kind");
    }
  }
};
} // namespace

std::unique_ptr<MCObjectTargetWriter>
llvm::createABCELFObjectWriter(uint8_t OSABI) {
  return std::make_unique<ABCELFObjectWriter>(OSABI);
}
