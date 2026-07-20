//===-- AVMMCInstLower.cpp - Lower AVM MachineInstr to MCInst -------------===//

#include "AVMMCInstLower.h"
#include "AVM.h"
#include "AVMInstrInfo.h"
#include "MCTargetDesc/AVMMCExpr.h"
#include "llvm/CodeGen/AsmPrinter.h"
#include "llvm/CodeGen/MachineBasicBlock.h"
#include "llvm/CodeGen/MachineInstr.h"
#include "llvm/MC/MCContext.h"
#include "llvm/MC/MCExpr.h"
#include "llvm/MC/MCInst.h"
#include "llvm/Support/ErrorHandling.h"

using namespace llvm;

void AVMMCInstLower::lower(const MachineInstr *MI, MCInst &OutMI) const {
  int Service = -1;
  switch (MI->getOpcode()) {
  case AVM::SYS_DEBUG_PUTC_PSEUDO:
    Service = 0x00;
    break;
  case AVM::SYS_DEBUG_BREAK_PSEUDO:
    Service = 0x01;
    break;
  case AVM::SYS_MILLIS_PSEUDO:
    Service = 0x02;
    break;
  case AVM::SYS_MILLIS32_PSEUDO:
    Service = 0x03;
    break;
  case AVM::SYS_SINF_PSEUDO:
    Service = 0x04;
    break;
  case AVM::SYS_COSF_PSEUDO:
    Service = 0x05;
    break;
  case AVM::SYS_ATAN2F_PSEUDO:
    Service = 0x06;
    break;
  case AVM::SYS_TANF_PSEUDO:
    Service = 0x07;
    break;
  case AVM::SYS_EXPF_PSEUDO:
    Service = 0x08;
    break;
  case AVM::SYS_LOGF_PSEUDO:
    Service = 0x09;
    break;
  case AVM::SYS_LOG2F_PSEUDO:
    Service = 0x0a;
    break;
  case AVM::SYS_LOG10F_PSEUDO:
    Service = 0x0b;
    break;
  case AVM::SYS_POWF_PSEUDO:
    Service = 0x0c;
    break;
  case AVM::SYS_HYPOTF_PSEUDO:
    Service = 0x0d;
    break;
  case AVM::SYS_FMODF_PSEUDO:
    Service = 0x0e;
    break;
  case AVM::SYS_MEMCPY_PSEUDO:
    Service = 0x0f;
    break;
  case AVM::SYS_MEMCPY_P_PSEUDO:
    Service = 0x10;
    break;
  case AVM::SYS_MEMSET_PSEUDO:
    Service = 0x11;
    break;
  case AVM::SYS_MEMMOVE_PSEUDO:
    Service = 0x12;
    break;
  default:
    break;
  }
  if (Service >= 0) {
    OutMI.setOpcode(AVM::SYS);
    OutMI.addOperand(MCOperand::createImm(Service));
    return;
  }

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
    case MachineOperand::MO_GlobalAddress: {
      const MCExpr *Expr =
          MCSymbolRefExpr::create(Printer.getSymbol(MO.getGlobal()), Ctx);
      if (MO.getOffset())
        Expr = MCBinaryExpr::createAdd(
            Expr, MCConstantExpr::create(MO.getOffset(), Ctx), Ctx);
      switch (MO.getTargetFlags()) {
      case AVMII::MO_NONE:
        break;
      case AVMII::MO_LO16:
        Expr = MCSpecifierExpr::create(Expr, AVM::VK_AVM_LO16, Ctx);
        break;
      case AVMII::MO_HI8:
        Expr = MCSpecifierExpr::create(Expr, AVM::VK_AVM_HI8, Ctx);
        break;
      default:
        llvm_unreachable("unsupported AVM target operand flag");
      }
      OutMI.addOperand(MCOperand::createExpr(Expr));
    } break;
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
