#include "MCTargetDesc/AVMMCTargetDesc.h"
#include "TargetInfo/AVMTargetInfo.h"
#include "llvm/ADT/ArrayRef.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/MC/MCDisassembler/MCSymbolizer.h"
#include "llvm/MC/MCExpr.h"
#include "llvm/MC/MCContext.h"
#include "llvm/MC/MCDisassembler/MCDisassembler.h"
#include "llvm/MC/MCInst.h"
#include "llvm/MC/TargetRegistry.h"
#include "llvm/Support/Compiler.h"

using namespace llvm;

namespace {

class AVMSymbolizer final : public MCSymbolizer {
  SectionSymbolsTy *Symbols;

public:
  AVMSymbolizer(MCContext &Ctx, std::unique_ptr<MCRelocationInfo> &&RelInfo,
                void *DisInfo)
      : MCSymbolizer(Ctx, std::move(RelInfo)),
        Symbols(static_cast<SectionSymbolsTy *>(DisInfo)) {}

  bool tryAddingSymbolicOperand(MCInst &Inst, raw_ostream &, int64_t Value,
                                uint64_t, bool IsBranch, uint64_t, uint64_t,
                                uint64_t) override {
    if (!IsBranch || !Symbols || Value < 0)
      return false;

    const uint64_t Target = static_cast<uint64_t>(Value);
    auto It = llvm::partition_point(*Symbols, [=](const SymbolInfoTy &Symbol) {
      return Symbol.Addr <= Target;
    });
    while (It != Symbols->begin()) {
      --It;
      if (It->IsMappingSymbol)
        continue;
      auto *Symbol = Ctx.getOrCreateSymbol(It->Name);
      const MCExpr *Expr = MCSymbolRefExpr::create(Symbol, Ctx);
      const uint64_t Offset = Target - It->Addr;
      if (Offset)
        Expr = MCBinaryExpr::createAdd(
            Expr, MCConstantExpr::create(Offset, Ctx), Ctx);
      Inst.addOperand(MCOperand::createExpr(Expr));
      return true;
    }
    return false;
  }

  void tryAddingPcLoadReferenceComment(raw_ostream &, int64_t,
                                       uint64_t) override {}
};

static MCSymbolizer *createAVMSymbolizer(
    const Triple &, LLVMOpInfoCallback, LLVMSymbolLookupCallback, void *DisInfo,
    MCContext *Ctx, std::unique_ptr<MCRelocationInfo> &&RelInfo) {
  return new AVMSymbolizer(*Ctx, std::move(RelInfo), DisInfo);
}

class AVMDisassembler final : public MCDisassembler {
  static constexpr uint64_t MaxProgramAddress = 0xffffff;

  void addDirectControlTarget(MCInst &MI, int64_t EncodedOperand,
                              uint64_t Address, uint64_t InstructionSize,
                              uint64_t OperandSize, bool IsRelative) const {
    int64_t Target = EncodedOperand;
    if (IsRelative) {
      if (Address > MaxProgramAddress) {
        MI.addOperand(MCOperand::createImm(EncodedOperand));
        return;
      }
      Target += static_cast<int64_t>(Address) + InstructionSize;
    }

    if (Target >= 0 && static_cast<uint64_t>(Target) <= MaxProgramAddress &&
        tryAddingSymbolicOperand(MI, Target, Address, /*IsBranch=*/true,
                                 /*Offset=*/1, OperandSize, InstructionSize))
      return;

    MI.addOperand(MCOperand::createImm(EncodedOperand));
  }

  static MCRegister compactRegister(unsigned Index) {
    switch (Index) {
    case 0: return AVM::R4;
    case 1: return AVM::R5;
    case 2: return AVM::R6;
    default: return AVM::R7;
    }
  }

  static MCRegister generalPointerRegister(unsigned Index) {
    switch (Index) {
    case 0: return AVM::R0;
    case 1: return AVM::R1;
    case 2: return AVM::R2;
    case 3: return AVM::R3;
    case 4: return AVM::R4;
    case 5: return AVM::R5;
    case 6: return AVM::R6;
    default: return AVM::R7;
    }
  }

  static MCRegister programPairRegister(unsigned Index) {
    switch (Index) {
    case 0: return AVM::R0R1;
    case 1: return AVM::R2R3;
    case 2: return AVM::R4R5;
    default: return AVM::R6R7;
    }
  }

  static bool decodeQPair12(unsigned Encoded, unsigned &D, unsigned &S) {
    if (Encoded >= 12)
      return false;
    if (Encoded < 8) {
      D = Encoded / 4;
      S = Encoded & 3;
    } else {
      D = 2 + (Encoded - 8) / 2;
      S = (Encoded - 8) & 1;
    }
    return true;
  }

  static bool decodePair48(unsigned Secondary, unsigned &Left,
                           unsigned &Right) {
    if (Secondary > 0x2f)
      return false;
    Left = Secondary < 0x20 ? Secondary / 8
                            : 4 + (Secondary - 0x20) / 4;
    Right = Secondary < 0x20 ? Secondary & 7
                             : (Secondary - 0x20) & 3;
    return true;
  }

  static std::optional<MCRegister> scalarRegisterFromPSPEC(unsigned Code) {
    switch (Code) {
    case 0x0: return AVM::R0;
    case 0x2: return AVM::R1;
    case 0x4: return AVM::R2;
    case 0x6: return AVM::R3;
    case 0x8: return AVM::R4;
    case 0xa: return AVM::R5;
    case 0xc: return AVM::R6;
    case 0xe: return AVM::R7;
    default: return std::nullopt;
    }
  }

  static std::optional<MCRegister> pairRegisterFromPSPEC(unsigned Code) {
    switch (Code) {
    case 0x0: return AVM::R0R1;
    case 0x4: return AVM::R2R3;
    case 0x8: return AVM::R4R5;
    case 0xc: return AVM::R6R7;
    default: return std::nullopt;
    }
  }

public:
  AVMDisassembler(const MCSubtargetInfo &STI, MCContext &Ctx)
      : MCDisassembler(STI, Ctx) {}

  DecodeStatus getInstruction(MCInst &MI, uint64_t &Size,
                              ArrayRef<uint8_t> Bytes, uint64_t Address,
                              raw_ostream &CStream) const override {
    CommentStream = &CStream;
    if (Bytes.empty())
      return Fail;

    if (Bytes[0] == 0xf0) {
      if (Bytes.size() < 2)
        return Fail;
      const uint8_t Secondary = Bytes[1];
      if (Secondary >= 0x60 && Secondary <= 0x68) {
        if (Bytes.size() < 3)
          return Fail;
        const bool IsPair = Secondary == 0x63 || Secondary == 0x64 ||
                            Secondary == 0x67 || Secondary == 0x68;
        const uint8_t PSPEC = Bytes[2];
        const auto Destination = IsPair
                                     ? pairRegisterFromPSPEC(PSPEC >> 4)
                                     : scalarRegisterFromPSPEC(PSPEC >> 4);
        const auto Address = pairRegisterFromPSPEC(PSPEC & 0xf);
        if (!Destination || !Address) {
          Size = 1;
          return Fail;
        }
        const bool PostIncrement = Secondary >= 0x65;
        if (PostIncrement &&
            (IsPair ? *Destination == *Address
                    : (PSPEC >> 6) == (PSPEC & 0xf) / 4)) {
          Size = 1;
          return Fail;
        }
        switch (Secondary) {
        case 0x60: MI.setOpcode(AVM::LDP8U); break;
        case 0x61: MI.setOpcode(AVM::LDP8S); break;
        case 0x62: MI.setOpcode(AVM::LDP16); break;
        case 0x63: MI.setOpcode(AVM::LDP24); break;
        case 0x64: MI.setOpcode(AVM::LDP32); break;
        case 0x65: MI.setOpcode(AVM::LDP8U_POST); break;
        case 0x66: MI.setOpcode(AVM::LDP16_POST); break;
        case 0x67: MI.setOpcode(AVM::LDP24_POST); break;
        default: MI.setOpcode(AVM::LDP32_POST); break;
        }
        MI.addOperand(MCOperand::createReg(*Destination));
        MI.addOperand(MCOperand::createReg(*Address));
        Size = 3;
        return Success;
      }
      if (Secondary >= 0x69 && Secondary <= 0x6b) {
        if (Bytes.size() < 3)
          return Fail;
        const uint8_t RRSPEC = Bytes[2];
        const auto High = pairRegisterFromPSPEC(RRSPEC >> 4);
        const auto Low = Secondary == 0x69
                             ? pairRegisterFromPSPEC(RRSPEC & 0xf)
                             : scalarRegisterFromPSPEC(RRSPEC & 0xf);
        if (!High || !Low) {
          Size = 1;
          return Fail;
        }
        if (Secondary == 0x69) {
          MI.setOpcode(AVM::CMP32);
          MI.addOperand(MCOperand::createReg(*High));
          MI.addOperand(MCOperand::createReg(*Low));
        } else if (Secondary == 0x6a) {
          MI.setOpcode(AVM::LD32);
          MI.addOperand(MCOperand::createReg(*High));
          MI.addOperand(MCOperand::createReg(*Low));
        } else {
          MI.setOpcode(AVM::ST32);
          MI.addOperand(MCOperand::createReg(*Low));
          MI.addOperand(MCOperand::createReg(*High));
        }
        Size = 3;
        return Success;
      }
      if (Secondary == 0x6c || Secondary == 0x6d) {
        if (Bytes.size() < 3)
          return Fail;
        const uint8_t Spec = Bytes[2];
        const unsigned DataIndex = Spec >> 5;
        const unsigned AddressIndex = (Spec >> 1) & 7;
        const bool IsWord = Spec & 0x10;
        const bool IsPost = Spec & 1;
        if (Secondary == 0x6c && IsPost && DataIndex == AddressIndex) {
          Size = 1;
          return Fail;
        }
        if (Secondary == 0x6c) {
          if (IsWord)
            MI.setOpcode(IsPost ? AVM::GPLD16_POST : AVM::GPLD16);
          else
            MI.setOpcode(IsPost ? AVM::GPLD8U_POST : AVM::GPLD8U);
        } else {
          if (IsWord)
            MI.setOpcode(IsPost ? AVM::GPST16_POST : AVM::GPST16);
          else
            MI.setOpcode(IsPost ? AVM::GPST8_POST : AVM::GPST8);
        }
        const MCRegister Data = generalPointerRegister(DataIndex);
        const MCRegister Address = generalPointerRegister(AddressIndex);
        if (Secondary == 0x6d)
          MI.addOperand(MCOperand::createReg(Address));
        MI.addOperand(MCOperand::createReg(Data));
        if (Secondary == 0x6c)
          MI.addOperand(MCOperand::createReg(Address));
        Size = 3;
        return Success;
      }
      if (Secondary >= 0x40 && Secondary <= 0x5f) {
        if (Bytes.size() < 4)
          return Fail;
        if (Secondary <= 0x47)
          MI.setOpcode(AVM::LDM8U);
        else if (Secondary <= 0x4f)
          MI.setOpcode(AVM::STM8);
        else if (Secondary <= 0x57)
          MI.setOpcode(AVM::LDM16);
        else
          MI.setOpcode(AVM::STM16);
        const MCRegister Reg = static_cast<MCRegister>(AVM::R0 + (Secondary & 7));
        const int64_t Address = Bytes[2] | (uint16_t(Bytes[3]) << 8);
        if ((Secondary >= 0x48 && Secondary <= 0x4f) || Secondary >= 0x58) {
          MI.addOperand(MCOperand::createImm(Address));
          MI.addOperand(MCOperand::createReg(Reg));
        } else {
          MI.addOperand(MCOperand::createReg(Reg));
          MI.addOperand(MCOperand::createImm(Address));
        }
        Size = 4;
        return Success;
      }
      if (Secondary > 0x3f) {
        Size = 1;
        return Fail;
      }
      const bool IsLDI16 = Secondary >= 0x04 && Secondary <= 0x07;
      if (Bytes.size() < (IsLDI16 ? 4 : 3))
        return Fail;
      if (Secondary <= 0x03)
        MI.setOpcode(AVM::COLDLDI8);
      else if (Secondary <= 0x07)
        MI.setOpcode(AVM::COLDLDI16);
      else if (Secondary <= 0x0b)
        MI.setOpcode(AVM::COLDADDIS8);
      else if (Secondary <= 0x0f)
        MI.setOpcode(AVM::COLDCMPIS8);
      else if (Secondary <= 0x17)
        MI.setOpcode(AVM::LEASP);
      else if (Secondary <= 0x1f)
        MI.setOpcode(AVM::LDSP8U);
      else if (Secondary <= 0x27)
        MI.setOpcode(AVM::LDSP8S);
      else if (Secondary <= 0x2f)
        MI.setOpcode(AVM::STSP8);
      else if (Secondary <= 0x37)
        MI.setOpcode(AVM::LDSP16);
      else
        MI.setOpcode(AVM::STSP16);
      const unsigned RegIndex = Secondary & (Secondary < 0x10 ? 3 : 7);
      const MCRegister Reg = static_cast<MCRegister>(AVM::R0 + RegIndex);
      const int64_t Value = IsLDI16
                                ? Bytes[2] | (uint16_t(Bytes[3]) << 8)
                                : ((Secondary >= 0x08 && Secondary <= 0x0f &&
                                    Bytes[2] & 0x80)
                                       ? int64_t(Bytes[2]) - 256
                                       : Bytes[2]);
      if (Secondary >= 0x28 && Secondary <= 0x2f || Secondary >= 0x38)
        MI.addOperand(MCOperand::createImm(Value));
      MI.addOperand(MCOperand::createReg(Reg));
      if (!(Secondary >= 0x28 && Secondary <= 0x2f || Secondary >= 0x38))
        MI.addOperand(MCOperand::createImm(Value));
      Size = IsLDI16 ? 4 : 3;
      return Success;
    }

    if (Bytes[0] == 0xf1) {
      if (Bytes.size() < 2) {
        Size = 1;
        return Fail;
      }
      const uint8_t Secondary = Bytes[1];
      if (Secondary >= 0x30 && Secondary <= 0x6f) {
        MI.setOpcode(AVM::STSP8_COMPACT);
        MI.addOperand(MCOperand::createImm((Secondary - 0x30) / 4));
        MI.addOperand(MCOperand::createReg(
            static_cast<MCRegister>(AVM::R4 + ((Secondary - 0x30) & 3))));
        Size = 2;
        return Success;
      }
      if (Secondary >= 0x70 && Secondary <= 0x8f) {
        const unsigned RegIndex = Secondary & 7;
        switch (Secondary & 0xf8) {
        case 0x70: MI.setOpcode(AVM::ZEXT8); break;
        case 0x78: MI.setOpcode(AVM::SWAP8); break;
        case 0x80: MI.setOpcode(AVM::GETSP); break;
        default: MI.setOpcode(AVM::SETSP); break;
        }
        MI.addOperand(MCOperand::createReg(
            static_cast<MCRegister>(AVM::R0 + RegIndex)));
        Size = 2;
        return Success;
      }
      if (Secondary > 0x2f) {
        Size = 1;
        return Fail;
      }
      unsigned D, S;
      decodePair48(Secondary, D, S);
      MI.setOpcode(AVM::MOV_RR);
      MI.addOperand(MCOperand::createReg(
          static_cast<MCRegister>(AVM::R0 + D)));
      MI.addOperand(MCOperand::createReg(
          static_cast<MCRegister>(AVM::R0 + S)));
      Size = 2;
      return Success;
    }

    if (Bytes[0] == 0xf2) {
      if (Bytes.size() < 2) {
        Size = 1;
        return Fail;
      }
      const uint8_t Secondary = Bytes[1];
      if (Secondary >= 0x60 && Secondary < 0x6c) {
        unsigned D, S;
        if (!decodeQPair12(Secondary - 0x60, D, S)) {
          Size = 1;
          return Fail;
        }
        MI.setOpcode(AVM::MOV32_F2);
        MI.addOperand(MCOperand::createReg(programPairRegister(D)));
        MI.addOperand(MCOperand::createReg(programPairRegister(S)));
        Size = 2;
        return Success;
      }
      if (Secondary > 0x5f) {
        Size = 1;
        return Fail;
      }
      const bool IsSub = Secondary >= 0x30;
      const unsigned Pair = IsSub ? Secondary - 0x30 : Secondary;
      unsigned D, S;
      decodePair48(Pair, D, S);
      MI.setOpcode(IsSub ? AVM::SUB_RR : AVM::ADD_RR);
      MI.addOperand(MCOperand::createReg(
          static_cast<MCRegister>(AVM::R0 + D)));
      MI.addOperand(MCOperand::createReg(
          static_cast<MCRegister>(AVM::R0 + S)));
      Size = 2;
      return Success;
    }

    if (Bytes[0] == 0xff) {
      if (Bytes.size() < 2) {
        Size = 1;
        return Fail;
      }
      const uint8_t Secondary = Bytes[1];
      if (Secondary < 0x60) {
        static constexpr unsigned Opcodes[] = {AVM::FADD, AVM::FSUB,
                                                AVM::FMUL, AVM::FDIV,
                                                AVM::FMIN, AVM::FMAX};
        MI.setOpcode(Opcodes[Secondary >> 4]);
        MI.addOperand(MCOperand::createReg(programPairRegister((Secondary >> 2) & 3)));
        MI.addOperand(MCOperand::createReg(programPairRegister(Secondary & 3)));
        Size = 2;
        return Success;
      }
      if (Secondary >= 0x60 && Secondary < 0x7c) {
        static constexpr unsigned Opcodes[] = {AVM::FNEG, AVM::FABS,
                                                AVM::FSQRT, AVM::FTRUNC,
                                                AVM::FFLOOR, AVM::FCEIL,
                                                AVM::FROUND};
        MI.setOpcode(Opcodes[(Secondary - 0x60) >> 2]);
        MI.addOperand(MCOperand::createReg(programPairRegister(Secondary & 3)));
        Size = 2;
        return Success;
      }
      if (Secondary < 0xc0 || Secondary > 0xc9) {
        Size = 1;
        return Fail;
      }
      if (Bytes.size() < 3) {
        Size = 2;
        return Fail;
      }
      const uint8_t Spec = Bytes[2];
      if ((Secondary <= 0xc3 || Secondary == 0xc9) && (Spec & 0x8c)) {
        Size = 1;
        return Fail;
      }
      if (Secondary >= 0xc4 && Secondary <= 0xc7 && (Spec & 0xf0)) {
        Size = 1;
        return Fail;
      }
      if (Secondary == 0xc8 && (Spec & 0x80)) {
        Size = 1;
        return Fail;
      }
      switch (Secondary) {
      case 0xc0: MI.setOpcode(AVM::S16TOF); break;
      case 0xc1: MI.setOpcode(AVM::U16TOF); break;
      case 0xc2: MI.setOpcode(AVM::FTOS16); break;
      case 0xc3: MI.setOpcode(AVM::FTOU16); break;
      case 0xc4: MI.setOpcode(AVM::S32TOF); break;
      case 0xc5: MI.setOpcode(AVM::U32TOF); break;
      case 0xc6: MI.setOpcode(AVM::FTOS32); break;
      case 0xc7: MI.setOpcode(AVM::FTOU32); break;
      case 0xc8: MI.setOpcode(AVM::FCMP); break;
      default: MI.setOpcode(AVM::FCLASS); break;
      }
      if (Secondary <= 0xc1) {
        MI.addOperand(MCOperand::createReg(programPairRegister(Spec & 3)));
        MI.addOperand(MCOperand::createReg(generalPointerRegister(Spec >> 4)));
      } else if (Secondary <= 0xc3 || Secondary == 0xc9) {
        MI.addOperand(MCOperand::createReg(generalPointerRegister(Spec >> 4)));
        MI.addOperand(MCOperand::createReg(programPairRegister(Spec & 3)));
      } else if (Secondary <= 0xc7) {
        MI.addOperand(MCOperand::createReg(programPairRegister((Spec >> 2) & 3)));
        MI.addOperand(MCOperand::createReg(programPairRegister(Spec & 3)));
      } else {
        MI.addOperand(MCOperand::createReg(generalPointerRegister((Spec >> 4) & 7)));
        MI.addOperand(MCOperand::createReg(programPairRegister((Spec >> 2) & 3)));
        MI.addOperand(MCOperand::createReg(programPairRegister(Spec & 3)));
      }
      Size = 3;
      return Success;
    }

    if (Bytes[0] == 0xf9) {
      if (Bytes.size() < 2) {
        Size = 1;
        return Fail;
      }
      const uint8_t Secondary = Bytes[1];
      switch (Secondary & 3) {
      case 0: MI.setOpcode(AVM::AND_RR); break;
      case 1: MI.setOpcode(AVM::OR_RR); break;
      case 2: MI.setOpcode(AVM::XOR_RR); break;
      default:
        Size = 1;
        return Fail;
      }
      MI.addOperand(MCOperand::createReg(generalPointerRegister(Secondary >> 5)));
      MI.addOperand(MCOperand::createReg(generalPointerRegister((Secondary >> 2) & 7)));
      Size = 2;
      return Success;
    }

    if (Bytes[0] == 0xfa) {
      if (Bytes.size() < 2)
        return Fail;
      const uint8_t Secondary = Bytes[1];
      if (Secondary >= 0xf0) {
        Size = 1;
        return Fail;
      }
      if (Secondary < 0x10) {
        MI.setOpcode(AVM::SHL16V);
        MI.addOperand(MCOperand::createReg(compactRegister(Secondary >> 2)));
        MI.addOperand(MCOperand::createReg(compactRegister(Secondary & 3)));
      } else if (Secondary < 0x20) {
        MI.setOpcode(AVM::LSR16V);
        MI.addOperand(MCOperand::createReg(compactRegister((Secondary - 0x10) >> 2)));
        MI.addOperand(MCOperand::createReg(compactRegister(Secondary & 3)));
      } else if (Secondary < 0x30) {
        MI.setOpcode(AVM::ASR16V);
        MI.addOperand(MCOperand::createReg(compactRegister((Secondary - 0x20) >> 2)));
        MI.addOperand(MCOperand::createReg(compactRegister(Secondary & 3)));
      } else {
        const unsigned Base = Secondary < 0x70 ? 0x30
                            : Secondary < 0xb0 ? 0x70 : 0xb0;
        MI.setOpcode(Base == 0x30 ? AVM::LSL16I
                     : Base == 0x70 ? AVM::LSR16I : AVM::ASR16I);
        MI.addOperand(MCOperand::createReg(compactRegister((Secondary - Base) >> 4)));
        MI.addOperand(MCOperand::createImm(Secondary & 0xf));
      }
      Size = 2;
      return Success;
    }

    if (Bytes[0] == 0xf5) {
      if (Bytes.size() < 2) {
        Size = 1;
        return Fail;
      }
      const uint8_t Secondary = Bytes[1];
      if (Secondary >= 0x30 && Secondary <= 0x3f) {
        MI.setOpcode(AVM::F5LD8U);
        MI.addOperand(MCOperand::createReg(
            static_cast<MCRegister>(AVM::R0 + (Secondary & 3))));
        MI.addOperand(MCOperand::createReg(
            compactRegister((Secondary - 0x30) / 4)));
        Size = 2;
        return Success;
      }
      if (Secondary >= 0x40 && Secondary <= 0x4f) {
        MI.setOpcode(AVM::F5LD16);
        MI.addOperand(MCOperand::createReg(
            static_cast<MCRegister>(AVM::R0 + (Secondary & 3))));
        MI.addOperand(MCOperand::createReg(
            compactRegister((Secondary - 0x40) / 4)));
        Size = 2;
        return Success;
      }
      if (Secondary >= 0x50 && Secondary <= 0x5f) {
        MI.setOpcode(AVM::F5ST16);
        MI.addOperand(MCOperand::createReg(
            compactRegister((Secondary - 0x50) / 4)));
        MI.addOperand(MCOperand::createReg(
            static_cast<MCRegister>(AVM::R0 + (Secondary & 3))));
        Size = 2;
        return Success;
      }
      if (Secondary > 0x2f) {
        Size = 1;
        return Fail;
      }
      unsigned L, R;
      decodePair48(Secondary, L, R);
      MI.setOpcode(AVM::CMP_RR);
      MI.addOperand(MCOperand::createReg(
          static_cast<MCRegister>(AVM::R0 + L)));
      MI.addOperand(MCOperand::createReg(
          static_cast<MCRegister>(AVM::R0 + R)));
      Size = 2;
      return Success;
    }

    if (Bytes[0] == 0xf6) {
      if (Bytes.size() < 2) {
        Size = 1;
        return Fail;
      }
      const uint8_t Secondary = Bytes[1];
      if (Secondary > 0x1f) {
        if (Secondary <= 0x27) {
          MI.setOpcode(AVM::BSWAP16);
          MI.addOperand(MCOperand::createReg(static_cast<MCRegister>(AVM::R0 + (Secondary - 0x20))));
        } else if (Secondary <= 0x2f) {
          MI.setOpcode(AVM::TST16);
          MI.addOperand(MCOperand::createReg(static_cast<MCRegister>(AVM::R0 + (Secondary - 0x28))));
        } else if (Secondary <= 0x3f) {
          MI.setOpcode(AVM::MUL8);
          MI.addOperand(MCOperand::createReg(compactRegister((Secondary - 0x30) / 4)));
          MI.addOperand(MCOperand::createReg(compactRegister((Secondary - 0x30) & 3)));
        } else if (Secondary <= 0x47) {
          MI.setOpcode(AVM::SEXT8);
          MI.addOperand(MCOperand::createReg(static_cast<MCRegister>(AVM::R0 + (Secondary - 0x40))));
        } else if (Secondary <= 0x4f) {
          MI.setOpcode(AVM::NEG16);
          MI.addOperand(MCOperand::createReg(static_cast<MCRegister>(AVM::R0 + (Secondary - 0x48))));
        } else {
          Size = 1;
          return Fail;
        }
        Size = 2;
        return Success;
      }
      MI.setOpcode(AVM::F6ST8_POST);
      MI.addOperand(MCOperand::createReg(compactRegister(Secondary / 8)));
      MI.addOperand(MCOperand::createReg(
          static_cast<MCRegister>(AVM::R0 + (Secondary & 7))));
      Size = 2;
      return Success;
    }

    if (Bytes[0] == 0xf7) {
      if (Bytes.size() < 2) {
        Size = 1;
        return Fail;
      }
      const uint8_t Secondary = Bytes[1];
      if (Secondary >= 0x60 && Secondary < 0x70) {
        MI.setOpcode(AVM::ADD32);
        MI.addOperand(MCOperand::createReg(programPairRegister((Secondary - 0x60) / 4)));
        MI.addOperand(MCOperand::createReg(programPairRegister((Secondary - 0x60) & 3)));
        Size = 2;
        return Success;
      } else if (Secondary >= 0x70 && Secondary < 0x80) {
        MI.setOpcode(AVM::SUB32);
        MI.addOperand(MCOperand::createReg(programPairRegister((Secondary - 0x70) / 4)));
        MI.addOperand(MCOperand::createReg(programPairRegister((Secondary - 0x70) & 3)));
        Size = 2;
        return Success;
      } else if (Secondary >= 0x80 && Secondary < 0x84) {
        MI.setOpcode(AVM::LSR32_1);
        MI.addOperand(MCOperand::createReg(programPairRegister(Secondary - 0x80)));
        Size = 2;
        return Success;
      } else if (Secondary >= 0x84 && Secondary < 0x88) {
        MI.setOpcode(AVM::ASR32_1);
        MI.addOperand(MCOperand::createReg(programPairRegister(Secondary - 0x84)));
        Size = 2;
        return Success;
      } else if (Secondary >= 0x88 && Secondary < 0x90) {
        MI.setOpcode(AVM::BOOL);
        MI.addOperand(MCOperand::createReg(generalPointerRegister(Secondary - 0x88)));
        Size = 2;
        return Success;
      } else if (Secondary > 0x5f) {
        Size = 1;
        return Fail;
      } else {
        const unsigned PointerIndex = Secondary / 8;
      const unsigned DataIndex = Secondary & 7;
      if (Secondary < 0x20 || (Secondary >= 0x20 && Secondary < 0x40)) {
        if (DataIndex == 4 + PointerIndex) {
          Size = 1;
          return Fail;
        }
        MI.setOpcode(Secondary < 0x20 ? AVM::F7LD8U_POST
                                     : AVM::F7LD16_POST);
        MI.addOperand(MCOperand::createReg(
            static_cast<MCRegister>(AVM::R0 + DataIndex)));
        MI.addOperand(MCOperand::createReg(compactRegister(PointerIndex)));
      } else {
        MI.setOpcode(AVM::F7ST16_POST);
        MI.addOperand(MCOperand::createReg(compactRegister(PointerIndex)));
        MI.addOperand(MCOperand::createReg(
            static_cast<MCRegister>(AVM::R0 + DataIndex)));
      }
      Size = 2;
      return Success;
      }
    }

    if (Bytes[0] == 0xf8) {
      if (Bytes.size() < 2) {
        Size = 1;
        return Fail;
      }
      const uint8_t Secondary = Bytes[1];
      if (Secondary >= 0x30) {
        Size = 1;
        return Fail;
      }
      const unsigned Family = Secondary & 0x38;
      const unsigned Reg = Secondary & 7;
      switch (Family) {
      case 0x00: MI.setOpcode(AVM::CSET_EQ); break;
      case 0x08: MI.setOpcode(AVM::CSET_NE); break;
      case 0x10: MI.setOpcode(AVM::CSET_ULT); break;
      case 0x18: MI.setOpcode(AVM::CSET_UGE); break;
      case 0x20: MI.setOpcode(AVM::CSET_SLT); break;
      default: MI.setOpcode(AVM::CSET_SGE); break;
      }
      MI.addOperand(MCOperand::createReg(
          static_cast<MCRegister>(AVM::R0 + Reg)));
      Size = 2;
      return Success;
    }

    if (Bytes[0] >= 0xfb && Bytes[0] <= 0xfd) {
      if (Bytes.size() < 2) {
        Size = 1;
        return Fail;
      }
      const uint8_t Secondary = Bytes[1];
      if (Secondary >= 0x80) {
        Size = 1;
        return Fail;
      }
      const unsigned D = (Secondary & 0x38) >> 3;
      const unsigned S = Secondary & 7;
      const bool Invert = (Secondary & 0x40) != 0;
      const unsigned Family = Bytes[0] - 0xfb;
      static constexpr unsigned Opcodes[3][2] = {
          {AVM::CMOV_EQ, AVM::CMOV_NE},
          {AVM::CMOV_ULT, AVM::CMOV_UGE},
          {AVM::CMOV_SLT, AVM::CMOV_SGE}};
      MI.setOpcode(Opcodes[Family][Invert]);
      MI.addOperand(MCOperand::createReg(
          static_cast<MCRegister>(AVM::R0 + D)));
      MI.addOperand(MCOperand::createReg(
          static_cast<MCRegister>(AVM::R0 + S)));
      Size = 2;
      return Success;
    }

    if (Bytes[0] == 0xfe) {
      if (Bytes.size() < 2) {
        Size = 1;
        return Fail;
      }
      const uint8_t Secondary = Bytes[1];
      if (Secondary >= 0x40) {
        Size = 1;
        return Fail;
      }
      const unsigned D = Secondary >> 3;
      const unsigned S = Secondary & 7;
      MI.setOpcode(AVM::MUL16);
      MI.addOperand(MCOperand::createReg(
          static_cast<MCRegister>(AVM::R0 + D)));
      MI.addOperand(MCOperand::createReg(
          static_cast<MCRegister>(AVM::R0 + S)));
      Size = 2;
      return Success;
    }

    if (Bytes[0] == 0xff) {
      Size = 1;
      return Fail;
    }

    if (Bytes[0] == 0xf3) {
      if (Bytes.size() < 2) {
        Size = 1;
        return Fail;
      }
      const uint8_t Secondary = Bytes[1];
      if (Secondary > 0x7f) {
        Size = 1;
        return Fail;
      }
      if (Secondary < 0x10) {
        MI.setOpcode(AVM::F3ST8);
        MI.addOperand(MCOperand::createReg(compactRegister(Secondary / 4)));
        MI.addOperand(MCOperand::createReg(
            static_cast<MCRegister>(AVM::R0 + (Secondary & 3))));
      } else {
        if (Secondary >= 0x40) {
          MI.setOpcode(AVM::LDSP8U_COMPACT);
          MI.addOperand(MCOperand::createReg(
              compactRegister((Secondary - 0x40) & 3)));
          MI.addOperand(MCOperand::createImm((Secondary - 0x40) / 4));
          Size = 2;
          return Success;
        }
        const unsigned Family = Secondary & 0x30;
        MI.setOpcode(Family == 0x10 ? AVM::MULU8W
                     : Family == 0x20 ? AVM::MULS8W : AVM::MULSU8W);
        const unsigned Matrix = Secondary & 0x0f;
        MI.addOperand(MCOperand::createReg(compactRegister(Matrix / 4)));
        MI.addOperand(MCOperand::createReg(compactRegister(Matrix & 3)));
      }
      Size = 2;
      return Success;
    }

    if (Bytes[0] == 0xf4) {
      if (Bytes.size() < 2) {
        Size = 1;
        return Fail;
      }
      const uint8_t Secondary = Bytes[1];
      if (Secondary > 0xb7) {
        Size = 1;
        return Fail;
      }
      if (Secondary >= 0x80) {
        static const unsigned Opcodes[] = {
            AVM::LSL16_1, AVM::LSR16_1, AVM::ASR16_1, AVM::NOT16,
            AVM::TST8, AVM::INC16, AVM::DEC16};
        const unsigned Family = (Secondary - 0x80) / 8;
        MI.setOpcode(Opcodes[Family]);
        MI.addOperand(MCOperand::createReg(
            static_cast<MCRegister>(AVM::R0 + ((Secondary - 0x80) & 7))));
        Size = 2;
        return Success;
      }
      const unsigned CompactIndex = Secondary & 3;
      const unsigned Offset = (Secondary & 0x3f) / 4;
      if (Secondary < 0x40) {
        MI.setOpcode(AVM::LDSP16_COMPACT);
        MI.addOperand(MCOperand::createReg(compactRegister(CompactIndex)));
        MI.addOperand(MCOperand::createImm(Offset));
      } else {
        MI.setOpcode(AVM::STSP16_COMPACT);
        MI.addOperand(MCOperand::createImm(Offset));
        MI.addOperand(MCOperand::createReg(compactRegister(CompactIndex)));
      }
      Size = 2;
      return Success;
    }

    if (Bytes[0] >= 0xb0 && Bytes[0] <= 0xbf) {
      MI.setOpcode(Bytes[0] < 0xb8 ? AVM::PUSH16 : AVM::POP16);
      MI.addOperand(MCOperand::createReg(
          static_cast<MCRegister>(AVM::R0 + (Bytes[0] & 7))));
      Size = 1;
      return Success;
    }

    if (Bytes[0] <= 0xaf) {
      switch (Bytes[0] >> 4) {
      case 0x0: MI.setOpcode(AVM::MOV); break;
      case 0x1: MI.setOpcode(AVM::ADD); break;
      case 0x2: MI.setOpcode(AVM::SUB); break;
      case 0x3: MI.setOpcode(AVM::CMP); break;
      case 0x4: MI.setOpcode(AVM::LD8U); break;
      case 0x5: MI.setOpcode(AVM::ST8); break;
      case 0x6: MI.setOpcode(AVM::LD16); break;
      case 0x7: MI.setOpcode(AVM::ST16); break;
      case 0x8: MI.setOpcode(AVM::AND); break;
      case 0x9: MI.setOpcode(AVM::OR); break;
      default: MI.setOpcode(AVM::XOR); break;
      }
      MI.addOperand(MCOperand::createReg(compactRegister((Bytes[0] >> 2) & 3)));
      MI.addOperand(MCOperand::createReg(compactRegister(Bytes[0] & 3)));
      Size = 1;
      return Success;
    }

    if (Bytes[0] >= 0xc0 && Bytes[0] <= 0xcf) {
      const uint8_t Opcode = Bytes[0];
      const bool IsLDI16 = Opcode >= 0xc4 && Opcode <= 0xc7;
      if (Bytes.size() < (IsLDI16 ? 3 : 2))
        return Fail;
      if (Opcode <= 0xc3)
        MI.setOpcode(AVM::LDI8);
      else if (IsLDI16)
        MI.setOpcode(AVM::LDI16);
      else if (Opcode <= 0xcb)
        MI.setOpcode(AVM::ADDIS8);
      else
        MI.setOpcode(AVM::CMPIS8);
      MI.addOperand(MCOperand::createReg(compactRegister(Opcode & 3)));
      const int64_t Immediate = IsLDI16
                                    ? Bytes[1] | (uint16_t(Bytes[2]) << 8)
                                    : (Opcode >= 0xc8 && Bytes[1] & 0x80
                                           ? int64_t(Bytes[1]) - 256
                                           : Bytes[1]);
      MI.addOperand(MCOperand::createImm(Immediate));
      Size = IsLDI16 ? 3 : 2;
      return Success;
    }

    if (Bytes[0] >= 0xd0 && Bytes[0] <= 0xd9) {
      if (Bytes.size() < 2)
        return Fail;
      switch (Bytes[0]) {
      case 0xd0: MI.setOpcode(AVM::BREQ8); break;
      case 0xd1: MI.setOpcode(AVM::BRNE8); break;
      case 0xd2: MI.setOpcode(AVM::BRULT8); break;
      case 0xd3: MI.setOpcode(AVM::BRSLT8); break;
      case 0xd4: MI.setOpcode(AVM::JMP8); break;
      case 0xd5: MI.setOpcode(AVM::CALL8); break;
      case 0xd8: MI.setOpcode(AVM::BRUGE8); break;
      case 0xd9: MI.setOpcode(AVM::BRSGE8); break;
      case 0xd6: MI.setOpcode(AVM::ADJSP); break;
      default:
        if (Bytes[1] > 3) {
          Size = 1;
          return Fail;
        }
        MI.setOpcode(AVM::SYS);
        MI.addOperand(MCOperand::createImm(Bytes[1]));
        Size = 2;
        return Success;
      }
      addDirectControlTarget(MI, int64_t(Bytes[1]) -
                                 ((Bytes[1] & 0x80) ? 256 : 0),
                             Address, /*InstructionSize=*/2,
                             /*OperandSize=*/1, /*IsRelative=*/true);
      Size = 2;
      return Success;
    }

    if (Bytes[0] >= 0xda && Bytes[0] <= 0xdf) {
      if (Bytes.size() < 3)
        return Fail;
      switch (Bytes[0]) {
      case 0xda: MI.setOpcode(AVM::BREQ16); break;
      case 0xdb: MI.setOpcode(AVM::BRNE16); break;
      case 0xdc: MI.setOpcode(AVM::BRULT16); break;
      case 0xdd: MI.setOpcode(AVM::BRUGE16); break;
      case 0xde: MI.setOpcode(AVM::BRSLT16); break;
      case 0xdf: MI.setOpcode(AVM::BRSGE16); break;
      }
      addDirectControlTarget(
          MI, int64_t(uint16_t(Bytes[1]) | (uint16_t(Bytes[2]) << 8)) -
                  ((Bytes[2] & 0x80) ? 65536 : 0),
          Address, /*InstructionSize=*/3, /*OperandSize=*/2,
          /*IsRelative=*/true);
      Size = 3;
      return Success;
    }

    if (Bytes[0] == 0xe0 || Bytes[0] == 0xe1) {
      if (Bytes.size() < 3)
        return Fail;
      MI.setOpcode(Bytes[0] == 0xe0 ? AVM::JMP16 : AVM::CALL16);
      addDirectControlTarget(
          MI, int64_t(uint16_t(Bytes[1]) | (uint16_t(Bytes[2]) << 8)) -
                  ((Bytes[2] & 0x80) ? 65536 : 0),
          Address, /*InstructionSize=*/3, /*OperandSize=*/2,
          /*IsRelative=*/true);
      Size = 3;
      return Success;
    }

    if (Bytes[0] == 0xe2 || Bytes[0] == 0xe3) {
      if (Bytes.size() < 4)
        return Fail;
      MI.setOpcode(Bytes[0] == 0xe2 ? AVM::JMPF : AVM::CALLF);
      addDirectControlTarget(MI, Bytes[1] | uint32_t(Bytes[2]) << 8 |
                                     uint32_t(Bytes[3]) << 16,
                             Address, /*InstructionSize=*/4,
                             /*OperandSize=*/3, /*IsRelative=*/false);
      Size = 4;
      return Success;
    }

    if (Bytes[0] >= 0xe4 && Bytes[0] <= 0xeb) {
      const unsigned Index = Bytes[0] & 3;
      static const MCRegister Pairs[] = {AVM::R0R1, AVM::R2R3,
                                         AVM::R4R5, AVM::R6R7};
      MI.setOpcode(Bytes[0] < 0xe8 ? AVM::JMPP : AVM::CALLP);
      MI.addOperand(MCOperand::createReg(Pairs[Index]));
      Size = 1;
      return Success;
    }

    if (Bytes[0] == 0xec) {
      if (Bytes.size() < 2)
        return Fail;
      const uint8_t Secondary = Bytes[1];
      static constexpr unsigned Opcodes[4] = {AVM::UDIV16, AVM::UREM16,
                                              AVM::SDIV16, AVM::SREM16};
      MI.setOpcode(Opcodes[Secondary >> 6]);
      MI.addOperand(MCOperand::createReg(
          static_cast<MCRegister>(AVM::R0 + ((Secondary >> 3) & 7))));
      MI.addOperand(MCOperand::createReg(
          static_cast<MCRegister>(AVM::R0 + (Secondary & 7))));
      Size = 2;
      return Success;
    }

    if (Bytes[0] == 0xef) {
      MI.setOpcode(AVM::RET);
      Size = 1;
      return Success;
    }

    {
      Size = 1;
      return Fail;
    }
  }
};

} // namespace

static MCDisassembler *createAVMDisassembler(const Target &,
                                              const MCSubtargetInfo &STI,
                                              MCContext &Ctx) {
  return new AVMDisassembler(STI, Ctx);
}

extern "C" LLVM_ABI LLVM_EXTERNAL_VISIBILITY void
LLVMInitializeAVMDisassembler() {
  TargetRegistry::RegisterMCDisassembler(getTheAVMTarget(),
                                          createAVMDisassembler);
  TargetRegistry::RegisterMCSymbolizer(getTheAVMTarget(), createAVMSymbolizer);
}
