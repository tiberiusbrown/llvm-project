#include "MCTargetDesc/AVMMCTargetDesc.h"
#include "TargetInfo/AVMTargetInfo.h"
#include "llvm/ADT/ArrayRef.h"
#include "llvm/MC/MCContext.h"
#include "llvm/MC/MCDisassembler/MCDisassembler.h"
#include "llvm/MC/MCInst.h"
#include "llvm/MC/TargetRegistry.h"
#include "llvm/Support/Compiler.h"

using namespace llvm;

namespace {

class AVMDisassembler final : public MCDisassembler {
  static MCRegister compactRegister(unsigned Index) {
    switch (Index) {
    case 0: return AVM::R4;
    case 1: return AVM::R5;
    case 2: return AVM::R6;
    default: return AVM::R7;
    }
  }

public:
  AVMDisassembler(const MCSubtargetInfo &STI, MCContext &Ctx)
      : MCDisassembler(STI, Ctx) {}

  DecodeStatus getInstruction(MCInst &MI, uint64_t &Size,
                              ArrayRef<uint8_t> Bytes, uint64_t,
                              raw_ostream &) const override {
    if (Bytes.empty())
      return Fail;

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

    if (Bytes[0] >= 0xd0 && Bytes[0] <= 0xd7) {
      if (Bytes.size() < 2)
        return Fail;
      switch (Bytes[0]) {
      case 0xd0: MI.setOpcode(AVM::BREQ); break;
      case 0xd1: MI.setOpcode(AVM::BRNE); break;
      case 0xd2: MI.setOpcode(AVM::BRULT); break;
      case 0xd3: MI.setOpcode(AVM::BRSLT); break;
      case 0xd4: MI.setOpcode(AVM::JMP); break;
      case 0xd5: MI.setOpcode(AVM::CALL); break;
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
      MI.addOperand(MCOperand::createImm(int64_t(Bytes[1]) -
          ((Bytes[1] & 0x80) ? 256 : 0)));
      Size = 2;
      return Success;
    }

    if (Bytes[0] >= 0xd8 && Bytes[0] <= 0xdf) {
      Size = 1;
      return Fail;
    }

    if (Bytes[0] == 0xe0 || Bytes[0] == 0xe1) {
      if (Bytes.size() < 3)
        return Fail;
      MI.setOpcode(Bytes[0] == 0xe0 ? AVM::JMP16 : AVM::CALL16);
      MI.addOperand(MCOperand::createImm(
          int64_t(uint16_t(Bytes[1]) | (uint16_t(Bytes[2]) << 8)) -
          ((Bytes[2] & 0x80) ? 65536 : 0)));
      Size = 3;
      return Success;
    }

    if (Bytes[0] == 0xe2 || Bytes[0] == 0xe3) {
      if (Bytes.size() < 4)
        return Fail;
      MI.setOpcode(Bytes[0] == 0xe2 ? AVM::JMPF : AVM::CALLF);
      MI.addOperand(MCOperand::createImm(Bytes[1] | uint32_t(Bytes[2]) << 8 |
                                         uint32_t(Bytes[3]) << 16));
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
}
