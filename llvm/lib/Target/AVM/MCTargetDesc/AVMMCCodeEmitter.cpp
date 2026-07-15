#include "AVMFixupKinds.h"
#include "AVMMCTargetDesc.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/MC/MCCodeEmitter.h"
#include "llvm/MC/MCContext.h"
#include "llvm/MC/MCFixup.h"
#include "llvm/MC/MCInst.h"
#include "llvm/Support/MathExtras.h"
#include <optional>

using namespace llvm;

namespace {

class AVMMCCodeEmitter final : public MCCodeEmitter {
  MCContext &Ctx;

  void error(const MCInst &MI, const Twine &Message) const {
    Ctx.reportError(MI.getLoc(), Message);
  }

  static void emit8(SmallVectorImpl<char> &Out, uint64_t Value) {
    Out.push_back(static_cast<char>(Value));
  }

  static void emit24(SmallVectorImpl<char> &Out, uint64_t Value) {
    emit8(Out, Value);
    emit8(Out, Value >> 8);
    emit8(Out, Value >> 16);
  }

  void emitFarTarget(const MCInst &MI, SmallVectorImpl<char> &Out,
                     SmallVectorImpl<MCFixup> &Fixups) const {
    const MCOperand &Operand = MI.getOperand(0);
    if (Operand.isImm()) {
      if (!isUInt<24>(Operand.getImm()))
        error(MI, "far target is out of 24-bit range");
      emit24(Out, Operand.getImm());
      return;
    }
    if (!Operand.isExpr()) {
      error(MI, "expected far target expression");
      emit24(Out, 0);
      return;
    }
    Fixups.push_back(
        MCFixup::create(1, Operand.getExpr(), AVM::fixup_avm_far24));
    emit24(Out, 0);
  }

  void emitRel8(const MCInst &MI, SmallVectorImpl<char> &Out,
                SmallVectorImpl<MCFixup> &Fixups, unsigned Opcode) const {
    if (MI.getNumOperands() != 1) {
      error(MI, "expected one relative displacement operand");
      return;
    }
    emit8(Out, Opcode);
    const MCOperand &Operand = MI.getOperand(0);
    if (Operand.isImm()) {
      if (!isInt<8>(Operand.getImm()))
        error(MI, "relative displacement is out of signed 8-bit range");
      emit8(Out, Operand.getImm());
      return;
    }
    if (!Operand.isExpr()) {
      error(MI, "expected relative displacement expression");
      emit8(Out, 0);
      return;
    }
    Fixups.push_back(
        MCFixup::create(1, Operand.getExpr(), AVM::fixup_avm_pcrel8, true));
    emit8(Out, 0);
  }

  void emitSigned8(const MCInst &MI, SmallVectorImpl<char> &Out,
                   unsigned Opcode) const {
    if (MI.getNumOperands() != 1 || !MI.getOperand(0).isImm()) {
      error(MI, "expected one signed 8-bit immediate operand");
      return;
    }
    const int64_t Value = MI.getOperand(0).getImm();
    if (!isInt<8>(Value)) {
      error(MI, "immediate is out of signed 8-bit range");
      return;
    }
    emit8(Out, Opcode);
    emit8(Out, Value);
  }

  void emitService(const MCInst &MI, SmallVectorImpl<char> &Out) const {
    if (MI.getNumOperands() != 1 || !MI.getOperand(0).isImm() ||
        MI.getOperand(0).getImm() < 0 || MI.getOperand(0).getImm() > 3) {
      error(MI, "invalid AVM version 1 service identifier");
      return;
    }
    emit8(Out, 0xd7);
    emit8(Out, MI.getOperand(0).getImm());
  }

  static std::optional<unsigned> compactRegIndex(MCRegister Reg) {
    switch (Reg.id()) {
    case AVM::R4: return 0;
    case AVM::R5: return 1;
    case AVM::R6: return 2;
    case AVM::R7: return 3;
    default: return std::nullopt;
    }
  }

  static std::optional<unsigned> stackRegIndex(MCRegister Reg) {
    switch (Reg.id()) {
    case AVM::R0: return 0;
    case AVM::R1: return 1;
    case AVM::R2: return 2;
    case AVM::R3: return 3;
    case AVM::R4: return 4;
    case AVM::R5: return 5;
    case AVM::R6: return 6;
    case AVM::R7: return 7;
    default: return std::nullopt;
    }
  }

  void emitStackReg(const MCInst &MI, SmallVectorImpl<char> &Out,
                    unsigned Family) const {
    if (MI.getNumOperands() != 1 || !MI.getOperand(0).isReg()) {
      error(MI, "expected one full register operand");
      return;
    }
    const std::optional<unsigned> Index =
        stackRegIndex(MI.getOperand(0).getReg());
    if (!Index) {
      error(MI, "expected full register r0-r7");
      return;
    }
    emit8(Out, Family | *Index);
  }

  void emitCompactMatrix(const MCInst &MI, SmallVectorImpl<char> &Out,
                         unsigned Family) const {
    if (MI.getNumOperands() != 2 || !MI.getOperand(0).isReg() ||
        !MI.getOperand(1).isReg()) {
      error(MI, "expected two compact register operands");
      return;
    }
    const std::optional<unsigned> First =
        compactRegIndex(MI.getOperand(0).getReg());
    const std::optional<unsigned> Second =
        compactRegIndex(MI.getOperand(1).getReg());
    if (!First || !Second) {
      error(MI, "expected compact register c0-c3");
      return;
    }
    emit8(Out, Family | (*First << 2) | *Second);
  }

  void emitCompactImmediate(const MCInst &MI, SmallVectorImpl<char> &Out,
                            unsigned Family, unsigned Bits, bool IsSigned) const {
    if (MI.getNumOperands() != 2 || !MI.getOperand(0).isReg() ||
        !MI.getOperand(1).isImm()) {
      error(MI, "expected compact register and immediate operands");
      return;
    }
    const std::optional<unsigned> Reg = compactRegIndex(MI.getOperand(0).getReg());
    if (!Reg) {
      error(MI, "expected compact register c0-c3");
      return;
    }
    const int64_t Value = MI.getOperand(1).getImm();
    const int64_t Min = IsSigned ? -(int64_t(1) << (Bits - 1)) : 0;
    const int64_t Max = IsSigned ? (int64_t(1) << (Bits - 1)) - 1
                                 : (int64_t(1) << Bits) - 1;
    if (Value < Min || Value > Max) {
      error(MI, "immediate is out of range");
      return;
    }
    emit8(Out, Family | *Reg);
    emit8(Out, Value);
    if (Bits == 16)
      emit8(Out, Value >> 8);
  }

public:
  explicit AVMMCCodeEmitter(MCContext &Ctx) : Ctx(Ctx) {}

  void encodeInstruction(const MCInst &MI, SmallVectorImpl<char> &Out,
                         SmallVectorImpl<MCFixup> &Fixups,
                         const MCSubtargetInfo &) const override {
    switch (MI.getOpcode()) {
    case AVM::MOV: emitCompactMatrix(MI, Out, 0x00); return;
    case AVM::ADD: emitCompactMatrix(MI, Out, 0x10); return;
    case AVM::SUB: emitCompactMatrix(MI, Out, 0x20); return;
    case AVM::CMP: emitCompactMatrix(MI, Out, 0x30); return;
    case AVM::LD8U: emitCompactMatrix(MI, Out, 0x40); return;
    case AVM::ST8: emitCompactMatrix(MI, Out, 0x50); return;
    case AVM::LD16: emitCompactMatrix(MI, Out, 0x60); return;
    case AVM::ST16: emitCompactMatrix(MI, Out, 0x70); return;
    case AVM::AND: emitCompactMatrix(MI, Out, 0x80); return;
    case AVM::OR: emitCompactMatrix(MI, Out, 0x90); return;
    case AVM::XOR: emitCompactMatrix(MI, Out, 0xa0); return;
    case AVM::PUSH16: emitStackReg(MI, Out, 0xb0); return;
    case AVM::POP16: emitStackReg(MI, Out, 0xb8); return;
    case AVM::LDI8: emitCompactImmediate(MI, Out, 0xc0, 8, false); return;
    case AVM::LDI16: emitCompactImmediate(MI, Out, 0xc4, 16, false); return;
    case AVM::ADDIS8: emitCompactImmediate(MI, Out, 0xc8, 8, true); return;
    case AVM::CMPIS8: emitCompactImmediate(MI, Out, 0xcc, 8, true); return;
    case AVM::BREQ: emitRel8(MI, Out, Fixups, 0xd0); return;
    case AVM::BRNE: emitRel8(MI, Out, Fixups, 0xd1); return;
    case AVM::BRULT: emitRel8(MI, Out, Fixups, 0xd2); return;
    case AVM::BRSLT: emitRel8(MI, Out, Fixups, 0xd3); return;
    case AVM::JMP: emitRel8(MI, Out, Fixups, 0xd4); return;
    case AVM::CALL: emitSigned8(MI, Out, 0xd5); return;
    case AVM::ADJSP: emitSigned8(MI, Out, 0xd6); return;
    case AVM::SYS: emitService(MI, Out); return;
    case AVM::JMPF:
      emit8(Out, 0xe2);
      emitFarTarget(MI, Out, Fixups);
      return;
    case AVM::CALLF:
      emit8(Out, 0xe3);
      emitFarTarget(MI, Out, Fixups);
      return;
    default:
      error(MI, "unsupported AVM MC opcode");
      return;
    }
  }
};

} // namespace

MCCodeEmitter *llvm::createAVMMCCodeEmitter(const MCInstrInfo &, MCContext &Ctx) {
  return new AVMMCCodeEmitter(Ctx);
}
