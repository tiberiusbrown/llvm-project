#include "AVMFixupKinds.h"
#include "AVMMCTargetDesc.h"
#include "llvm/ADT/ArrayRef.h"
#include "llvm/BinaryFormat/ELF.h"
#include "llvm/MC/MCAsmBackend.h"
#include "llvm/MC/MCContext.h"
#include "llvm/MC/MCELFObjectWriter.h"
#include "llvm/MC/MCObjectWriter.h"
#include "llvm/MC/MCTargetOptions.h"
#include "llvm/Support/MathExtras.h"
#include "llvm/Support/raw_ostream.h"
#include <cassert>
#include <memory>

using namespace llvm;

namespace {

class AVMELFObjectWriter final : public MCELFObjectTargetWriter {
public:
  explicit AVMELFObjectWriter(uint8_t OSABI)
      : MCELFObjectTargetWriter(false, OSABI, ELF::EM_AVM,
                                /*HasRelocationAddend=*/true) {}

  unsigned getRelocType(const MCFixup &Fixup, const MCValue &,
                        bool) const override {
    switch (Fixup.getKind()) {
    case FK_Data_2:
    case AVM::fixup_avm_data16: return ELF::R_AVM_DATA16;
    case AVM::fixup_avm_prog24: return ELF::R_AVM_PROG24;
    case AVM::fixup_avm_prog_lo16: return ELF::R_AVM_PROG_LO16;
    case AVM::fixup_avm_prog_hi8: return ELF::R_AVM_PROG_HI8;
    case AVM::fixup_avm_pcrel8: return ELF::R_AVM_PCREL8;
    case AVM::fixup_avm_bank16: return ELF::R_AVM_BANK16;
    case AVM::fixup_avm_far24: return ELF::R_AVM_FAR24;
    case AVM::fixup_avm_relax: return ELF::R_AVM_RELAX;
    default:
      llvm_unreachable("unsupported AVM fixup kind");
    }
  }
};

class AVMAsmBackend final : public MCAsmBackend {
  uint8_t OSABI;

public:
  explicit AVMAsmBackend(uint8_t OSABI)
      : MCAsmBackend(endianness::little), OSABI(OSABI) {}

  std::unique_ptr<MCObjectTargetWriter>
  createObjectTargetWriter() const override {
    return createAVMELFObjectWriter(OSABI);
  }

  MCFixupKindInfo getFixupKindInfo(MCFixupKind Kind) const override {
    static const MCFixupKindInfo Infos[AVM::NumTargetFixupKinds] = {
        {"fixup_avm_data16", 0, 16, 0},
        {"fixup_avm_prog24", 0, 24, 0},
        {"fixup_avm_prog_lo16", 0, 16, 0},
        {"fixup_avm_prog_hi8", 0, 8, 0},
        {"fixup_avm_pcrel8", 0, 8, 0},
        {"fixup_avm_bank16", 0, 16, 0},
        {"fixup_avm_far24", 0, 24, 0},
        {"fixup_avm_relax", 0, 0, 0},
    };
    if (Kind < FirstTargetFixupKind)
      return MCAsmBackend::getFixupKindInfo(Kind);
    unsigned Index = Kind - FirstTargetFixupKind;
    assert(Index < AVM::NumTargetFixupKinds && "invalid AVM fixup kind");
    return Infos[Index];
  }

  void applyFixup(const MCFragment &F, const MCFixup &Fixup,
                  const MCValue &Target, uint8_t *Data, uint64_t Value,
                  bool IsResolved) override {
    maybeAddReloc(F, Fixup, Target, Value, IsResolved);
    if (!IsResolved)
      return;

    auto Error = [&](const Twine &Message) {
      getContext().reportError(Fixup.getLoc(), Message);
    };

    switch (Fixup.getKind()) {
    case AVM::fixup_avm_data16:
      if (!isUInt<16>(Value))
        Error("AVM data-space relocation is out of 16-bit range");
      Data[0] = Value;
      Data[1] = Value >> 8;
      return;
    case AVM::fixup_avm_prog24:
      if (!isUInt<24>(Value))
        Error("AVM program-space relocation is out of 24-bit range");
      Data[0] = Value;
      Data[1] = Value >> 8;
      Data[2] = Value >> 16;
      return;
    case AVM::fixup_avm_prog_lo16:
    case AVM::fixup_avm_bank16:
      if (!isUInt<24>(Value))
        Error("AVM program address is out of 24-bit range");
      Data[0] = Value;
      Data[1] = Value >> 8;
      return;
    case AVM::fixup_avm_prog_hi8:
      if (!isUInt<24>(Value))
        Error("AVM program address is out of 24-bit range");
      Data[0] = Value >> 16;
      return;
    case AVM::fixup_avm_pcrel8: {
      int64_t Signed = static_cast<int64_t>(Value);
      if (!isInt<8>(Signed))
        Error("AVM relative displacement is out of signed 8-bit range");
      Data[0] = static_cast<uint8_t>(Signed);
      return;
    }
    case AVM::fixup_avm_far24: {
      if (!isUInt<24>(Value))
        Error("AVM far target is out of 24-bit range");
      if (Value & 1)
        Error("AVM far target must be two-byte aligned");
      uint8_t Link = Data[0] & 1;
      Data[0] = static_cast<uint8_t>(Value & 0xfe) | Link;
      Data[1] = Value >> 8;
      Data[2] = Value >> 16;
      return;
    }
    case AVM::fixup_avm_relax:
      return;
    default:
      llvm_unreachable("unknown AVM fixup");
    }
  }

  unsigned getMinimumNopSize() const override { return 2; }
  unsigned getMaximumNopSize(const MCSubtargetInfo &) const override {
    return 2;
  }

  bool writeNopData(raw_ostream &OS, uint64_t Count,
                    const MCSubtargetInfo *) const override {
    if (Count & 1)
      return false;
    while (Count) {
      OS.write("\xf4\xf3", 2);
      Count -= 2;
    }
    return true;
  }
};

} // namespace

std::unique_ptr<MCObjectTargetWriter>
llvm::createAVMELFObjectWriter(uint8_t OSABI) {
  return std::make_unique<AVMELFObjectWriter>(OSABI);
}

MCAsmBackend *llvm::createAVMAsmBackend(const Target &,
                                        const MCSubtargetInfo &,
                                        const MCRegisterInfo &,
                                        const MCTargetOptions &) {
  return new AVMAsmBackend(ELF::ELFOSABI_NONE);
}
