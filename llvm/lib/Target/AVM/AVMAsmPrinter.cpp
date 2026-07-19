//===-- AVMAsmPrinter.cpp - AVM assembly and object emission --------------===//

#include "AVM.h"
#include "AVMMCInstLower.h"
#include "AVMTargetMachine.h"
#include "MCTargetDesc/AVMMCTargetDesc.h"
#include "TargetInfo/AVMTargetInfo.h"
#include "llvm/CodeGen/AsmPrinter.h"
#include "llvm/CodeGen/MachineInstr.h"
#include "llvm/MC/MCInst.h"
#include "llvm/MC/MCStreamer.h"
#include "llvm/MC/TargetRegistry.h"
#include "llvm/Support/Compiler.h"

using namespace llvm;

#define DEBUG_TYPE "avm-asm-printer"

namespace {
class AVMAsmPrinter final : public AsmPrinter {
public:
  AVMAsmPrinter(TargetMachine &TM, std::unique_ptr<MCStreamer> Streamer)
      : AsmPrinter(TM, std::move(Streamer), ID) {}

  StringRef getPassName() const override { return "AVM Assembly Printer"; }

  void emitInstruction(const MachineInstr *MI) override {
    AVM_MC::verifyInstructionPredicates(MI->getOpcode(),
                                        getSubtargetInfo().getFeatureBits());
    AVMMCInstLower Lowering(OutContext, *this);
    MCInst OutMI;
    Lowering.lower(MI, OutMI);
    EmitToStreamer(*OutStreamer, OutMI);
  }

  static char ID;
};
} // namespace

char AVMAsmPrinter::ID = 0;

INITIALIZE_PASS(AVMAsmPrinter, DEBUG_TYPE, "AVM Assembly Printer", false, false)

extern "C" LLVM_ABI LLVM_EXTERNAL_VISIBILITY void
LLVMInitializeAVMAsmPrinter() {
  RegisterAsmPrinter<AVMAsmPrinter> X(getTheAVMTarget());
}
