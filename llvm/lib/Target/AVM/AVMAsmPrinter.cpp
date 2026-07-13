#include "AVMTargetMachine.h"
#include "MCTargetDesc/AVMMCTargetDesc.h"
#include "TargetInfo/AVMTargetInfo.h"
#include "llvm/CodeGen/AsmPrinter.h"
#include "llvm/CodeGen/MachineInstr.h"
#include "llvm/MC/MCExpr.h"
#include "llvm/MC/MCInst.h"
#include "llvm/MC/MCStreamer.h"
#include "llvm/MC/TargetRegistry.h"
#include "llvm/Support/ErrorHandling.h"

using namespace llvm;

static const MCExpr *withOffset(const MCExpr *Expr, int64_t Offset,
                                MCContext &Ctx) {
  if (!Offset)
    return Expr;
  return MCBinaryExpr::createAdd(Expr, MCConstantExpr::create(Offset, Ctx),
                                 Ctx);
}

namespace {
class AVMAsmPrinter final : public AsmPrinter {
public:
  static char ID;
  AVMAsmPrinter(TargetMachine &TM, std::unique_ptr<MCStreamer> Streamer)
      : AsmPrinter(TM, std::move(Streamer), ID) {}

  StringRef getPassName() const override { return "AVM Assembly Printer"; }
  void emitInstruction(const MachineInstr *MI) override;
};
} // namespace

void AVMAsmPrinter::emitInstruction(const MachineInstr *MI) {
  MCInst Out;
  Out.setOpcode(MI->getOpcode());
  for (unsigned I = 0; I != MI->getNumOperands(); ++I) {
    const MachineOperand &MO = MI->getOperand(I);
    if (MO.isRegMask() ||
        (MO.isReg() &&
         (MO.isImplicit() || (MO.isUse() && MI->isRegTiedToDefOperand(I)))))
      continue;
    switch (MO.getType()) {
    case MachineOperand::MO_Register:
      if (MO.getReg())
        Out.addOperand(MCOperand::createReg(MO.getReg()));
      break;
    case MachineOperand::MO_Immediate:
      Out.addOperand(MCOperand::createImm(MO.getImm()));
      break;
    case MachineOperand::MO_CImmediate:
      Out.addOperand(MCOperand::createImm(MO.getCImm()->getSExtValue()));
      break;
    case MachineOperand::MO_MachineBasicBlock:
      Out.addOperand(MCOperand::createExpr(
          MCSymbolRefExpr::create(MO.getMBB()->getSymbol(), OutContext)));
      break;
    case MachineOperand::MO_GlobalAddress:
      Out.addOperand(MCOperand::createExpr(withOffset(
          MCSymbolRefExpr::create(getSymbol(MO.getGlobal()), OutContext),
          MO.getOffset(), OutContext)));
      break;
    case MachineOperand::MO_ExternalSymbol:
      Out.addOperand(MCOperand::createExpr(MCSymbolRefExpr::create(
          GetExternalSymbolSymbol(MO.getSymbolName()), OutContext)));
      break;
    default:
      report_fatal_error("unsupported AVM machine operand in assembly printer");
    }
  }
  EmitToStreamer(*OutStreamer, Out);
}

char AVMAsmPrinter::ID = 0;

extern "C" LLVM_ABI LLVM_EXTERNAL_VISIBILITY void
LLVMInitializeAVMAsmPrinter() {
  RegisterAsmPrinter<AVMAsmPrinter> X(getTheAVMTarget());
}
