//===-- ABCMCCodeEmitter.cpp - Convert ABC MCInst to bytes ----------------===//

#include "ABCFixupKinds.h"
#include "ABCInstrFormats.h"
#include "ABCMCTargetDesc.h"
#include "llvm/MC/MCCodeEmitter.h"
#include "llvm/MC/MCExpr.h"
#include "llvm/MC/MCFixup.h"
#include "llvm/MC/MCInst.h"
#include "llvm/Support/raw_ostream.h"

using namespace llvm;

namespace {
class ABCMCCodeEmitter : public MCCodeEmitter {
public:
  void encodeInstruction(const MCInst &Inst, SmallVectorImpl<char> &CB,
                         SmallVectorImpl<MCFixup> &Fixups,
                         const MCSubtargetInfo &STI) const override;

private:
  void emitByte(SmallVectorImpl<char> &CB, uint8_t Byte) const {
    CB.push_back(static_cast<char>(Byte));
  }
  MCFixupKind getFixupKind(ABC::Fixup Fixup) const;
  void emitImm(SmallVectorImpl<char> &CB, SmallVectorImpl<MCFixup> &Fixups,
               const MCOperand &Op, ABC::ImmWidth Width,
               ABC::Fixup Fixup) const;
};
} // namespace

MCFixupKind ABCMCCodeEmitter::getFixupKind(ABC::Fixup Fixup) const {
  switch (Fixup) {
  case ABC::Fixup::Abs8:
    return MCFixupKind(ABC::fixup_abc_8);
  case ABC::Fixup::Abs16:
    return MCFixupKind(ABC::fixup_abc_16);
  case ABC::Fixup::Abs24:
    return MCFixupKind(ABC::fixup_abc_24);
  case ABC::Fixup::Abs32:
    return MCFixupKind(ABC::fixup_abc_32);
  case ABC::Fixup::Prog24:
    return MCFixupKind(ABC::fixup_abc_prog24);
  case ABC::Fixup::Global16Tagged:
    return MCFixupKind(ABC::fixup_abc_global16_tagged);
  case ABC::Fixup::Global8:
    return MCFixupKind(ABC::fixup_abc_global8);
  case ABC::Fixup::Branch8:
    return MCFixupKind(ABC::fixup_abc_branch8);
  case ABC::Fixup::Branch16:
    return MCFixupKind(ABC::fixup_abc_branch16);
  case ABC::Fixup::Call24:
    return MCFixupKind(ABC::fixup_abc_call24);
  case ABC::Fixup::None:
    llvm_unreachable("ABC instruction operand is not relocatable");
  }
  llvm_unreachable("unknown ABC fixup selector");
}

void ABCMCCodeEmitter::emitImm(SmallVectorImpl<char> &CB,
                               SmallVectorImpl<MCFixup> &Fixups,
                               const MCOperand &Op, ABC::ImmWidth Width,
                               ABC::Fixup Fixup) const {
  unsigned Bytes = static_cast<unsigned>(Width);
  if (Op.isExpr()) {
    Fixups.push_back(MCFixup::create(CB.size(), Op.getExpr(),
                                     getFixupKind(Fixup)));
    for (unsigned I = 0; I < Bytes; ++I)
      emitByte(CB, 0);
    return;
  }

  uint64_t Value = Op.getImm();
  for (unsigned I = 0; I < Bytes; ++I)
    emitByte(CB, (Value >> (I * 8)) & 0xff);
}

void ABCMCCodeEmitter::encodeInstruction(const MCInst &Inst,
                                         SmallVectorImpl<char> &CB,
                                         SmallVectorImpl<MCFixup> &Fixups,
                                         const MCSubtargetInfo &STI) const {
  const ABC::InstrDesc *Desc = ABC::getInstrDescByOpcode(Inst.getOpcode());
  if (!Desc)
    llvm_unreachable("unknown ABC opcode");

  emitByte(CB, Desc->Bytecode);
  for (unsigned I = 0; I < Desc->NumOperands; ++I)
    emitImm(CB, Fixups, Inst.getOperand(I), Desc->Widths[I],
            Desc->Fixups[I]);
}

MCCodeEmitter *llvm::createABCMCCodeEmitter(const MCInstrInfo &MCII,
                                            MCContext &Ctx) {
  return new ABCMCCodeEmitter();
}
