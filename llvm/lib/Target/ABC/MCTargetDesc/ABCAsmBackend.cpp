//===-- ABCAsmBackend.cpp - ABC assembler backend -------------------------===//

#include "ABCFixupKinds.h"
#include "ABCMCTargetDesc.h"
#include "llvm/BinaryFormat/ELF.h"
#include "llvm/MC/MCAsmBackend.h"
#include "llvm/MC/MCObjectWriter.h"
#include "llvm/MC/MCSubtargetInfo.h"
#include "llvm/Support/Endian.h"

using namespace llvm;

namespace {
class ABCAsmBackend : public MCAsmBackend {
public:
  ABCAsmBackend() : MCAsmBackend(endianness::little) {}

  std::unique_ptr<MCObjectTargetWriter>
  createObjectTargetWriter() const override {
    return createABCELFObjectWriter(ELF::ELFOSABI_STANDALONE);
  }

  MCFixupKindInfo getFixupKindInfo(MCFixupKind Kind) const override {
    if (Kind < FirstTargetFixupKind)
      return MCAsmBackend::getFixupKindInfo(Kind);

    static const MCFixupKindInfo Infos[ABC::NumTargetFixupKinds] = {
        {"fixup_abc_8", 0, 8, 0},
        {"fixup_abc_16", 0, 16, 0},
        {"fixup_abc_24", 0, 24, 0},
        {"fixup_abc_32", 0, 32, 0},
        {"fixup_abc_prog24", 0, 24, 0},
        {"fixup_abc_global16_tagged", 0, 16, 0},
        {"fixup_abc_global8", 0, 8, 0},
        {"fixup_abc_branch8", 0, 8, 0},
        {"fixup_abc_branch16", 0, 16, 0},
        {"fixup_abc_call24", 0, 24, 0},
    };
    return Infos[Kind - FirstTargetFixupKind];
  }

  void applyFixup(const MCFragment &F, const MCFixup &Fixup,
                  const MCValue &Target, uint8_t *Data, uint64_t Value,
                  bool IsResolved) override {
    maybeAddReloc(F, Fixup, Target, Value, IsResolved);

    MCFixupKindInfo Info = getFixupKindInfo(Fixup.getKind());
    unsigned NumBytes = (Info.TargetSize + 7) / 8;
    for (unsigned I = 0; I < NumBytes; ++I)
      Data[I] |= uint8_t(Value >> (I * 8));
  }

  bool writeNopData(raw_ostream &OS, uint64_t Count,
                    const MCSubtargetInfo *STI) const override {
    for (uint64_t I = 0; I < Count; ++I)
      OS << char(0);
    return true;
  }
};
} // namespace

MCAsmBackend *llvm::createABCAsmBackend(const Target &T,
                                        const MCSubtargetInfo &STI,
                                        const MCRegisterInfo &MRI,
                                        const MCTargetOptions &Options) {
  return new ABCAsmBackend();
}
