//===-- ABCDisassembler.cpp - Disassembler for ABC ------------------------===//

#include "MCTargetDesc/ABCMCTargetDesc.h"
#include "MCTargetDesc/ABCInstrFormats.h"
#include "TargetInfo/ABCTargetInfo.h"
#include "llvm/MC/MCContext.h"
#include "llvm/MC/MCDisassembler/MCDisassembler.h"
#include "llvm/MC/MCInst.h"
#include "llvm/MC/MCSubtargetInfo.h"
#include "llvm/MC/TargetRegistry.h"
#include "llvm/Support/Compiler.h"

using namespace llvm;

namespace {
class ABCDisassembler : public MCDisassembler {
public:
  ABCDisassembler(const MCSubtargetInfo &STI, MCContext &Ctx)
      : MCDisassembler(STI, Ctx) {}

  DecodeStatus getInstruction(MCInst &MI, uint64_t &Size,
                              ArrayRef<uint8_t> Bytes, uint64_t Address,
                              raw_ostream &CStream) const override;
};
} // namespace

static uint32_t readUnsigned(ArrayRef<uint8_t> Bytes, ABC::ImmWidth Width) {
  uint32_t Value = 0;
  for (unsigned I = 0, E = static_cast<unsigned>(Width); I != E; ++I)
    Value |= uint32_t(Bytes[I]) << (I * 8);
  return Value;
}

static int32_t signExtend(uint32_t Value, ABC::ImmWidth Width) {
  unsigned Bits = static_cast<unsigned>(Width) * 8;
  uint32_t SignBit = 1u << (Bits - 1);
  return int32_t((Value ^ SignBit) - SignBit);
}

MCDisassembler::DecodeStatus
ABCDisassembler::getInstruction(MCInst &MI, uint64_t &Size,
                                ArrayRef<uint8_t> Bytes, uint64_t Address,
                                raw_ostream &CStream) const {
  if (Bytes.empty())
    return Fail;

  const ABC::InstrDesc *Desc = ABC::getInstrDescByBytecode(Bytes[0]);
  if (!Desc)
    return Fail;

  Size = 1;
  for (unsigned I = 0; I < Desc->NumOperands; ++I)
    Size += static_cast<unsigned>(Desc->Widths[I]);
  if (Bytes.size() < Size)
    return Fail;

  MI.setOpcode(Desc->Opcode);
  uint64_t Offset = 1;
  for (unsigned I = 0; I < Desc->NumOperands; ++I) {
    ABC::ImmWidth Width = Desc->Widths[I];
    uint32_t Value = readUnsigned(Bytes.drop_front(Offset), Width);
    if (Desc->Fixups[I] == ABC::Fixup::Branch8 ||
        Desc->Fixups[I] == ABC::Fixup::Branch16)
      MI.addOperand(MCOperand::createImm(signExtend(Value, Width)));
    else
      MI.addOperand(MCOperand::createImm(Value));
    Offset += static_cast<unsigned>(Width);
  }
  return Success;
}

static MCDisassembler *createABCDisassembler(const Target &T,
                                             const MCSubtargetInfo &STI,
                                             MCContext &Ctx) {
  return new ABCDisassembler(STI, Ctx);
}

extern "C" LLVM_ABI LLVM_EXTERNAL_VISIBILITY void
LLVMInitializeABCDisassembler() {
  TargetRegistry::RegisterMCDisassembler(getTheABCTarget(),
                                         createABCDisassembler);
}
