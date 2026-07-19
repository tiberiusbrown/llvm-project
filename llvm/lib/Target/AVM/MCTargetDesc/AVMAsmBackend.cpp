#include "AVMFixupKinds.h"
#include "AVMMCExpr.h"
#include "AVMMCTargetDesc.h"
#include "llvm/ADT/ArrayRef.h"
#include "llvm/BinaryFormat/ELF.h"
#include "llvm/MC/MCAsmBackend.h"
#include "llvm/MC/MCContext.h"
#include "llvm/MC/MCELFObjectWriter.h"
#include "llvm/MC/MCObjectWriter.h"
#include "llvm/MC/MCTargetOptions.h"
#include "llvm/MC/MCValue.h"
#include "llvm/Support/MathExtras.h"
#include <cassert>
#include <memory>

using namespace llvm;

namespace {

class AVMELFObjectWriter final : public MCELFObjectTargetWriter {
public:
  explicit AVMELFObjectWriter(uint8_t OSABI)
      : MCELFObjectTargetWriter(false, OSABI, ELF::EM_AVM,
                                /*HasRelocationAddend=*/true) {}

  unsigned getRelocType(const MCFixup &Fixup, const MCValue &Target, bool) const override {
    if (Fixup.getKind() == FK_Data_1 && Target.getSpecifier() == AVM::VK_AVM_HI8)
      return ELF::R_AVM_PROG_HI8;
    if (Fixup.getKind() == FK_Data_2) {
      if (Target.getSpecifier() == AVM::VK_AVM_LO16)
        return ELF::R_AVM_PROG_LO16;
      if (!Target.getSpecifier()) return ELF::R_AVM_DATA16;
    }
    switch (Fixup.getKind()) {
    case AVM::fixup_avm_pcrel8:
      return ELF::R_AVM_PCREL8;
    case AVM::fixup_avm_pcrel16:
      return ELF::R_AVM_PCREL16;
    case AVM::fixup_avm_far24:
      return ELF::R_AVM_FAR24;
    case AVM::fixup_avm_data16:
      return ELF::R_AVM_DATA16;
    case AVM::fixup_avm_prog24:
      return ELF::R_AVM_PROG24;
    case AVM::fixup_avm_prog_lo16:
      return ELF::R_AVM_PROG_LO16;
    case AVM::fixup_avm_prog_hi8:
      return ELF::R_AVM_PROG_HI8;
    case AVM::fixup_avm_relax:
      return ELF::R_AVM_RELAX;
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
        {"fixup_avm_pcrel8", 0, 8, 0},
        {"fixup_avm_pcrel16", 0, 16, 0},
        {"fixup_avm_far24", 0, 24, 0},
        {"fixup_avm_data16", 0, 16, 0},
        {"fixup_avm_prog24", 0, 24, 0},
        {"fixup_avm_prog_lo16", 0, 16, 0},
        {"fixup_avm_prog_hi8", 0, 8, 0},
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
    // Logical program addresses are assigned after MC writes its input object.
    if (IsResolved && Fixup.getKind() == AVM::fixup_avm_far24)
      IsResolved = false;
    if (IsResolved && (Fixup.getKind() == AVM::fixup_avm_data16 ||
                       Fixup.getKind() == AVM::fixup_avm_prog24 ||
                       Fixup.getKind() == AVM::fixup_avm_prog_lo16 ||
                       Fixup.getKind() == AVM::fixup_avm_prog_hi8 ||
                       Fixup.getKind() == FK_Data_1 || Fixup.getKind() == FK_Data_2))
      IsResolved = false;
    if (IsResolved && Fixup.getKind() == AVM::fixup_avm_relax)
      IsResolved = false;
    maybeAddReloc(F, Fixup, Target, Value, IsResolved);
    if (!IsResolved)
      return;

    auto Error = [&](const Twine &Message) {
      getContext().reportError(Fixup.getLoc(), Message);
    };
    switch (Fixup.getKind()) {
    case AVM::fixup_avm_pcrel8: {
      // MC's PC-relative value is based on the operand byte at P + 1;
      // AVM rel8 is defined from the two-byte instruction's next PC.
      int64_t Signed = static_cast<int64_t>(Value) - 1;
      if (!isInt<8>(Signed))
        Error("AVM relative displacement is out of signed 8-bit range");
      Data[0] = static_cast<uint8_t>(Signed);
      return;
    }
    case AVM::fixup_avm_pcrel16: {
      // MC's PC-relative value is based on the rel16 field at P + 1;
      // AVM rel16 is defined from this three-byte instruction's next PC.
      int64_t Signed = static_cast<int64_t>(Value) - 2;
      if (!isInt<16>(Signed))
        Error("AVM relative displacement is out of signed 16-bit range");
      Data[0] = static_cast<uint8_t>(Signed);
      Data[1] = static_cast<uint8_t>(Signed >> 8);
      return;
    }
    case AVM::fixup_avm_far24:
      if (!isUInt<24>(Value))
        Error("AVM far target is out of 24-bit range");
      Data[0] = static_cast<uint8_t>(Value);
      Data[1] = static_cast<uint8_t>(Value >> 8);
      Data[2] = static_cast<uint8_t>(Value >> 16);
      return;
    case AVM::fixup_avm_data16:
      if (!isUInt<16>(Value))
        Error("AVM absolute data address is out of unsigned 16-bit range");
      Data[0] = static_cast<uint8_t>(Value);
      Data[1] = static_cast<uint8_t>(Value >> 8);
      return;
    case AVM::fixup_avm_prog24:
      if (!isUInt<24>(Value)) Error("AVM program address is out of 24-bit range");
      Data[0] = static_cast<uint8_t>(Value); Data[1] = static_cast<uint8_t>(Value >> 8);
      Data[2] = static_cast<uint8_t>(Value >> 16); return;
    case AVM::fixup_avm_prog_lo16:
      if (!isUInt<24>(Value)) Error("AVM program address is out of 24-bit range");
      Data[0] = static_cast<uint8_t>(Value); Data[1] = static_cast<uint8_t>(Value >> 8); return;
    case AVM::fixup_avm_prog_hi8:
      if (!isUInt<24>(Value)) Error("AVM program address is out of 24-bit range");
      Data[0] = static_cast<uint8_t>(Value >> 16); return;
    case AVM::fixup_avm_relax:
      return;
    default:
      llvm_unreachable("unknown AVM fixup");
    }
  }

  bool writeNopData(raw_ostream &OS, uint64_t Count,
                    const MCSubtargetInfo *) const override {
    // There is no retained NOP encoding before the opcode-map implementation.
    OS.write_zeros(Count);
    return true;
  }
};

} // namespace

std::unique_ptr<MCObjectTargetWriter>
llvm::createAVMELFObjectWriter(uint8_t OSABI) {
  return std::make_unique<AVMELFObjectWriter>(OSABI);
}

MCAsmBackend *llvm::createAVMAsmBackend(const Target &, const MCSubtargetInfo &,
                                        const MCRegisterInfo &,
                                        const MCTargetOptions &) {
  return new AVMAsmBackend(ELF::ELFOSABI_NONE);
}
