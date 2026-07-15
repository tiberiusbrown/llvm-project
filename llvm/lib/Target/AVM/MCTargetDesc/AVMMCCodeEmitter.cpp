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

  static std::optional<unsigned> compactRegIndex(MCRegister Reg) {
    switch (Reg.id()) {
    case AVM::R4: return 0;
    case AVM::R5: return 1;
    case AVM::R6: return 2;
    case AVM::R7: return 3;
    default: return std::nullopt;
    }
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
