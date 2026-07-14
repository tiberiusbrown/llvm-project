//===-- ABCAsmPrinter.cpp - ABC assembly printer --------------------------===//

#include "ABC.h"
#include "MCTargetDesc/ABCMCTargetDesc.h"
#include "TargetInfo/ABCTargetInfo.h"
#include "llvm/CodeGen/AsmPrinter.h"
#include "llvm/CodeGen/MachineInstr.h"
#include "llvm/MC/MCInst.h"
#include "llvm/MC/MCStreamer.h"
#include "llvm/MC/TargetRegistry.h"

using namespace llvm;

#define DEBUG_TYPE "abc-asm-printer"

namespace {
class ABCAsmPrinter : public AsmPrinter {
public:
  ABCAsmPrinter(TargetMachine &TM, std::unique_ptr<MCStreamer> Streamer)
      : AsmPrinter(TM, std::move(Streamer)) {}

  StringRef getPassName() const override { return "ABC Assembly Printer"; }

  void emitInstruction(const MachineInstr *MI) override {
    MCInst TmpInst;
    TmpInst.setOpcode(MI->getOpcode());
    EmitToStreamer(*OutStreamer, TmpInst);
  }
};
} // namespace

extern "C" LLVM_ABI LLVM_EXTERNAL_VISIBILITY void
LLVMInitializeABCAsmPrinter() {
  RegisterAsmPrinter<ABCAsmPrinter> X(getTheABCTarget());
}
