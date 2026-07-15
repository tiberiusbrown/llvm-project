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
public:
  AVMDisassembler(const MCSubtargetInfo &STI, MCContext &Ctx)
      : MCDisassembler(STI, Ctx) {}

  DecodeStatus getInstruction(MCInst &MI, uint64_t &Size,
                              ArrayRef<uint8_t> Bytes, uint64_t,
                              raw_ostream &) const override {
    if (Bytes.size() < 4)
      return Fail;
    if (Bytes[0] != 0xe2 && Bytes[0] != 0xe3) {
      Size = 1;
      return Fail;
    }
    MI.setOpcode(Bytes[0] == 0xe2 ? AVM::JMPF : AVM::CALLF);
    MI.addOperand(MCOperand::createImm(Bytes[1] | uint32_t(Bytes[2]) << 8 |
                                       uint32_t(Bytes[3]) << 16));
    Size = 4;
    return Success;
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
