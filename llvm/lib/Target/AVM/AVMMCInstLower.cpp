//===-- AVMMCInstLower.cpp - Lower AVM MachineInstr to MCInst -------------===//

#include "AVMMCInstLower.h"
#include "llvm/CodeGen/AsmPrinter.h"
#include "llvm/CodeGen/MachineBasicBlock.h"
#include "llvm/CodeGen/MachineInstr.h"
#include "llvm/MC/MCContext.h"
#include "llvm/MC/MCExpr.h"
#include "llvm/MC/MCInst.h"
#include "llvm/Support/ErrorHandling.h"

using namespace llvm;

void AVMMCInstLower::lower(const MachineInstr *MI, MCInst &OutMI) const {
  OutMI.setOpcode(MI->getOpcode());

  for (unsigned I = 0; I != MI->getNumOperands(); ++I) {
    const MachineOperand &MO = MI->getOperand(I);
    if ((MO.isReg() && MO.isImplicit()) || MO.isRegMask())
      continue;
    if (I < MI->getDesc().getNumOperands() &&
        MI->getDesc().getOperandConstraint(I, MCOI::TIED_TO) >= 0)
      continue;

    switch (MO.getType()) {
    case MachineOperand::MO_Register:
      OutMI.addOperand(MCOperand::createReg(MO.getReg()));
      break;
    case MachineOperand::MO_Immediate:
      OutMI.addOperand(MCOperand::createImm(MO.getImm()));
      break;
    case MachineOperand::MO_MachineBasicBlock:
      OutMI.addOperand(MCOperand::createExpr(
          MCSymbolRefExpr::create(MO.getMBB()->getSymbol(), Ctx)));
      break;
    case MachineOperand::MO_GlobalAddress:
      OutMI.addOperand(MCOperand::createExpr(
          MCSymbolRefExpr::create(Printer.getSymbol(MO.getGlobal()), Ctx)));
      break;
    case MachineOperand::MO_ExternalSymbol:
      OutMI.addOperand(MCOperand::createExpr(MCSymbolRefExpr::create(
          Printer.GetExternalSymbolSymbol(MO.getSymbolName()), Ctx)));
      break;
    default:
      MI->print(errs());
      llvm_unreachable("unsupported AVM machine operand");
    }
  }
}
