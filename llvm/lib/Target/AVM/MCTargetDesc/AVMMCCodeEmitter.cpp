#include "AVMFixupKinds.h"
#include "AVMMCTargetDesc.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/MC/MCCodeEmitter.h"
#include "llvm/MC/MCContext.h"
#include "llvm/MC/MCFixup.h"
#include "llvm/MC/MCInst.h"
#include "llvm/Support/MathExtras.h"

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

public:
  explicit AVMMCCodeEmitter(MCContext &Ctx) : Ctx(Ctx) {}

  void encodeInstruction(const MCInst &MI, SmallVectorImpl<char> &Out,
                         SmallVectorImpl<MCFixup> &Fixups,
                         const MCSubtargetInfo &) const override {
    switch (MI.getOpcode()) {
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
